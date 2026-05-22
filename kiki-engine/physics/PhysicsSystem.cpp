#include "physics/PhysicsSystem.hpp"
#include "physics/PhysicsDebugRenderer.hpp"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/ScaleHelpers.h>

#include "ECS/World.h"
#include <spdlog/spdlog.h>

namespace Kiki {

    void PhysicsSystem::OnStart() {

        _manager.Initialize();

        auto& reg = World::Get().Registry();

        //Bind EnTT signal
        reg.on_construct<RigidBodyComponent>().connect<&PhysicsSystem::OnRigidBodyCreated>(this);
        reg.on_destroy<RigidBodyComponent>().connect<&PhysicsSystem::OnRigidBodyDestroyed>(this);

        reg.ctx().emplace<PhysicsService>(_manager);

        spdlog::info("PhysicsSystem started and signals connected.");
    }

    void PhysicsSystem::OnUpdate(float dt) {
        ZoneScopedN("Updating physics system");

        auto& reg = World::Get().Registry();
        auto& bodyInterface = _manager.GetBodyInterface();
        auto view = reg.view<TransformComponent, RigidBodyComponent, PhysicalAttributesComponent>();
        for (auto [entity, transform, rb,ip] : view.each()) {
            if (transform.dirty) {
                if (rb.motionType == JPH::EMotionType::Kinematic) {
                    if (dt > 0.0f) {
                        bodyInterface.MoveKinematic(
                            rb.bodyID,
                            ToJPHR(transform.position),
                            ToJPH(transform.rotation),
                            dt
                        );
                    }
                }
                else {
                    JPH::RVec3 jPos = bodyInterface.GetPosition(rb.bodyID);
                    if (glm::distance(transform.position, ToGLM(jPos)) > 0.01f) {
                        bodyInterface.SetPosition(
                            rb.bodyID, ToJPHR(transform.position), JPH::EActivation::Activate
                        );
                    }
                    bodyInterface.SetRotation(
                        rb.bodyID, ToJPH(transform.rotation), JPH::EActivation::Activate
                    );
                }
            }
            if (ip.impulse != glm::vec3(0) ) {
                bodyInterface.AddImpulse(rb.bodyID, ToJPH(ip.impulse));
				ip.impulse = glm::vec3(0);
            }
        }

        // Pure physics simulation
        _manager.Update(dt);

		auto& debugRenderer = PhysicsDebugRenderer::get();
		debugRenderer.clear();
		if (debugRenderer.enabled) {
            if (debugRenderer.drawBodies) {
                JPH::BodyManager::DrawSettings settings;
                settings.mDrawShape = true;
                settings.mDrawShapeWireframe = true;
                settings.mDrawBoundingBox = debugRenderer.drawBoundingBoxes;
                settings.mDrawVelocity = debugRenderer.drawVelocity;
                _manager.GetSystem()->DrawBodies(settings, &debugRenderer);
            }
        }

        //write back the results to ECS
        for (auto [entity, transform, rb, ip] : view.each()) {
            if (rb.motionType == JPH::EMotionType::Dynamic) {
                bool isActive = bodyInterface.IsActive(rb.bodyID);

                if (isActive) {
                    JPH::Vec3 vel = bodyInterface.GetLinearVelocity(rb.bodyID);
                    bool needsClamp = false;

                    float horizontalSpeedSq = vel.GetX() * vel.GetX() + vel.GetZ() * vel.GetZ();
                    float maxSafeHorizontalSq = 1600.0f; // (40m/s)^2

                    if (horizontalSpeedSq > maxSafeHorizontalSq) {
                        vel.SetX(0.0f);
                        vel.SetZ(0.0f);
                        needsClamp = true;
                    }

                    float maxSafeUpwardSpeed = 10.0f;

                    if (vel.GetY() > maxSafeUpwardSpeed) {
                        vel.SetY(0.0f);
                        needsClamp = true;
                    }

                    if (needsClamp) {
                        bodyInterface.SetLinearVelocity(rb.bodyID, vel);
                        //spdlog::warn("=== [PHYSICS CLAMP] Clamped! H-Sq: {:.2f}, V-Up: {:.2f} ===", horizontalSpeedSq, vel.GetY());
                    }

                    JPH::RVec3 pos; JPH::Quat rot;
                    bodyInterface.GetPositionAndRotation(rb.bodyID, pos, rot);
                    transform.position = ToGLM(pos);
                    transform.rotation = ToGLM(rot);
                    transform.dirty = true;
                }
                else {
                    static bool loggedSleep = false;
                    if (!loggedSleep) {
                        //spdlog::warn("=== [PHYSICS DEACTIVATED] Entity {} is now SLEEPING. ===", (uint32_t)entity);
                        loggedSleep = true;
                    }
                }
            }

            if (rb.motionType == JPH::EMotionType::Dynamic) {
                UpdateIsGrounded(entity);
            }
        }
    }

    void PhysicsSystem::OnStop() {
        _manager.Shutdown();
        spdlog::info("PhysicsSystem shut down.");
    }

    //void PhysicsSystem::AddImpulse(entt::entity entity, const glm::vec3& impulse) {
    //    auto& reg = World::Get().Registry();
    //    if (auto* rb = reg.try_get<RigidBodyComponent>(entity)) {
    //        if (!rb->bodyID.IsInvalid()) {
    //            _manager.GetBodyInterface().AddImpulse(rb->bodyID, ToJPH(impulse));
    //            spdlog::info("impulse!!!");
    //        }
    //    }
    //}

    //void PhysicsSystem::AddForce(entt::entity entity, const glm::vec3& force) {
    //    auto& reg = World::Get().Registry();
    //    if (auto* rb = reg.try_get<RigidBodyComponent>(entity)) {
    //        if (!rb->bodyID.IsInvalid()) {
    //            _manager.GetBodyInterface().AddForce(rb->bodyID, ToJPH(force));
    //            spdlog::info("force!!!");
    //        }
    //    }
    //}

    void PhysicsSystem::OnRigidBodyCreated(entt::registry& reg, entt::entity entity) {
        auto& rb = reg.get<RigidBodyComponent>(entity);
        auto& transform = reg.get<TransformComponent>(entity);

        JPH::ShapeRefC shape;
        bool requiresUniformScale = false; // capsule + sphere can't accept non-uniform scale

        if (auto* meshColl = reg.try_get<MeshColliderComponent>(entity)) {
            shape = meshColl->shape;
            spdlog::info("Entity {} using pre-computed MeshCollider.", (uint32_t)entity);
        }
        else if (auto* capsule = reg.try_get<CapsuleColliderComponent>(entity)) {
            JPH::Ref<JPH::Shape> baseCapsule = new JPH::CapsuleShape(capsule->halfHeight, capsule->radius);

            // calculate the offset to move the capsule up so that its bottom is at the Transform's position
            float offsetUp = capsule->halfHeight + capsule->radius;

            JPH::RotatedTranslatedShapeSettings offsetShapeSettings(
                JPH::Vec3(0, offsetUp, 0),
                JPH::Quat::sIdentity(),
                baseCapsule
            );

            shape = offsetShapeSettings.Create().Get();
            requiresUniformScale = true;
        }
        else if (auto* box = reg.try_get<BoxColliderComponent>(entity)) {
            shape = new JPH::BoxShape(ToJPH(box->halfExtents));
        }
        else if (auto* sphere = reg.try_get<SphereColliderComponent>(entity)) {
            shape = new JPH::SphereShape(sphere->radius);
            requiresUniformScale = true;
        }
        else {
            shape = new JPH::BoxShape(JPH::Vec3(0.5f, 0.5f, 0.5f));
            spdlog::warn("Entity {} has RigidBody but no Collider. Created default box shape.", (uint32_t)entity);
        }

        // Wrap the shape with a ScaledShape so transform.scale (e.g. coming from
        // glTF parent nodes that haven't had their scale applied) propagates to
        // the physics body. Without this, the shape uses the raw mesh-local
        // vertices while rendering uses T*R*S, so the collision box mysteriously
        // shifts / shrinks relative to what you see on screen.
        {
            JPH::Vec3 scaleVec = ToJPH(transform.scale);
            if (!JPH::ScaleHelpers::IsNotScaled(scaleVec)) {
                if (requiresUniformScale && !JPH::ScaleHelpers::IsUniformScale(scaleVec)) {
                    JPH::Vec3 uniform = JPH::ScaleHelpers::MakeUniformScale(scaleVec);
                    spdlog::warn(
                        "Entity {}: shape requires uniform scale but transform.scale = ({:.3f}, {:.3f}, {:.3f}); forcing uniform = {:.3f}",
                        (uint32_t)entity,
                        scaleVec.GetX(), scaleVec.GetY(), scaleVec.GetZ(),
                        uniform.GetX());
                    scaleVec = uniform;
                }
                if (JPH::ScaleHelpers::IsZeroScale(scaleVec)) {
                    scaleVec = JPH::ScaleHelpers::MakeNonZeroScale(scaleVec);
                }
                shape = new JPH::ScaledShape(shape, scaleVec);
            }
        }

        JPH::BodyCreationSettings settings(
            shape,
            ToJPHR(transform.position),
            ToJPH(transform.rotation),
            rb.motionType,
            rb.layer
        );

        settings.mFriction = rb.friction;
        settings.mRestitution = rb.restitution;
        settings.mIsSensor = rb.isSensor;

        if (rb.lockRotationXZ) {
            settings.mAllowedDOFs = JPH::EAllowedDOFs::TranslationX |
                JPH::EAllowedDOFs::TranslationY |
                JPH::EAllowedDOFs::TranslationZ;
        }
        else {
            settings.mAllowedDOFs = JPH::EAllowedDOFs::All;
        }

        if (rb.motionType == JPH::EMotionType::Dynamic) {
            settings.mMotionQuality = JPH::EMotionQuality::LinearCast;
        }

        JPH::MassProperties massProps = settings.GetShape()->GetMassProperties();

        if (massProps.mMass <= 0.0f && rb.motionType != JPH::EMotionType::Static) {

            massProps.mMass = 1000.0f;
            massProps.mInertia = JPH::Mat44::sIdentity();
            settings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
        }

        settings.mMassPropertiesOverride = massProps;

        auto& bi = _manager.GetBodyInterface();
        JPH::Body* body = bi.CreateBody(settings);
        bi.AddBody(body->GetID(), JPH::EActivation::Activate);

        rb.bodyID = body->GetID();
        body->SetUserData((JPH::uint64)entity);
    }

    void PhysicsSystem::OnRigidBodyDestroyed(entt::registry& reg, entt::entity entity) {
        auto& rb = reg.get<RigidBodyComponent>(entity);
        if (!rb.bodyID.IsInvalid()) {
            auto& bi = _manager.GetBodyInterface();
            bi.RemoveBody(rb.bodyID);
            bi.DestroyBody(rb.bodyID);
        }
    }

    RaycastHit PhysicsSystem::Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, JPH::BodyID ignoreID) {
        RaycastHit result;
        result.hasHit = false;
        result.entity = entt::null;

        JPH::RRayCast ray;
        ray.mOrigin = ToJPHR(origin);
        ray.mDirection = ToJPH(direction * maxDistance);

        JPH::IgnoreSingleBodyFilter bodyFilter(ignoreID);
        JPH::RayCastResult joltResult;

        bool hit = _manager.GetSystem()->GetNarrowPhaseQuery().CastRay(
            ray,
            joltResult,
            {}, {},
            bodyFilter
        );

        if (hit) {
            result.hasHit = true;
            result.distance = joltResult.mFraction * maxDistance;
            result.position = origin + direction * result.distance;

            JPH::uint64 userData = _manager.GetBodyInterface().GetUserData(joltResult.mBodyID);
            result.entity = (entt::entity)userData;

            JPH::BodyLockRead lock(_manager.GetSystem()->GetBodyLockInterface(), joltResult.mBodyID);
            if (lock.Succeeded()) {
                result.normal = ToGLM(lock.GetBody().GetWorldSpaceSurfaceNormal(joltResult.mSubShapeID2, ray.GetPointOnRay(joltResult.mFraction)));
            }
        }
        return result;
    }

    void PhysicsSystem::UpdateIsGrounded(entt::entity entity, float maxDistance) {
        auto& reg = World::Get().Registry();
        auto* ip = reg.try_get<PhysicalAttributesComponent>(entity);
        if (!ip) return;

        glm::vec3 rayOrigin = reg.get<TransformComponent>(entity).position;
        rayOrigin.y += 0.3f;

        float testRayLength = 0.6f;

        JPH::RRayCast ray;
        ray.mOrigin = ToJPHR(rayOrigin);
        ray.mDirection = ToJPH(glm::vec3(0, -1, 0) * testRayLength);

        JPH::IgnoreSingleBodyFilter bodyFilter(reg.get<RigidBodyComponent>(entity).bodyID);
        JPH::RayCastResult joltResult;

        bool hit = _manager.GetSystem()->GetNarrowPhaseQuery().CastRay(
            ray, joltResult, {}, {}, bodyFilter
        );

        ip->isGrounded = hit;

        if (hit) {
            JPH::uint64 userData = _manager.GetBodyInterface().GetUserData(joltResult.mBodyID);
            entt::entity hitEntity = (entt::entity)userData;

			glm::vec3 hitPoint = rayOrigin + glm::vec3(0, -1, 0) * (joltResult.mFraction * testRayLength);

            if (hitEntity != entt::null && reg.valid(hitEntity)) {
                if (auto* groundRb = reg.try_get<RigidBodyComponent>(hitEntity)) {
                    JPH::Vec3 gVel = _manager.GetBodyInterface().GetLinearVelocity(groundRb->bodyID);
                    ip->groundVelocity = ToGLM(gVel);

					JPH::Vec3 pVel = _manager.GetBodyInterface().GetPointVelocity(groundRb->bodyID, ToJPHR(hitPoint));
					ip->PointVelocity = ToGLM(pVel);
                }
            }
        }
        else {
            ip->groundVelocity = glm::vec3(0.0f);
            ip->PointVelocity = glm::vec3(0.0f);
        }
    }

} // namespace Kiki