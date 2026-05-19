#pragma once

#include "ECS/System.h"
#include "physics/PhysicsManager.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include "physics/PhysicsUtils.hpp"
#include "physics/PhysicsComponents.hpp"
namespace Kiki {

    struct RaycastHit {
        bool hasHit = false;
        entt::entity entity = entt::null;
        glm::vec3 position{ 0.0f };
        glm::vec3 normal{ 0.0f };
        float distance = 0.0f;
    };
    struct PhysicsService {
        PhysicsService(PhysicsManager& physicsManager)
            : _manager(physicsManager) {}
        PhysicsManager& _manager;
        RaycastHit Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, JPH::BodyID ignoreID = JPH::BodyID(), bool needNormal = false) {
            RaycastHit result;

            JPH::RRayCast ray;
            ray.mOrigin = ToJPHR(origin);
            ray.mDirection = ToJPH(direction * maxDistance);

            JPH::IgnoreSingleBodyFilter bodyFilter(ignoreID);

            JPH::RayCastResult joltResult;

            bool hit = _manager.GetSystem()->GetNarrowPhaseQuery().CastRay(
                ray,
                joltResult,
                {},
                {},
                bodyFilter
            );

            if (hit) {
                result.hasHit = true;
                result.distance = joltResult.mFraction * maxDistance;
                result.position = origin + direction * result.distance;
                if (needNormal) {
                    auto& reg = World::Get().Registry();
                    auto view = reg.view<RigidBodyComponent>();
                    for (auto [ent, rb] : view.each()) {
                        if (rb.bodyID == joltResult.mBodyID) {
                            result.entity = ent;

                            JPH::BodyLockRead lock(_manager.GetSystem()->GetBodyLockInterface(), joltResult.mBodyID);
                            if (lock.Succeeded()) {
                                result.normal = ToGLM(lock.GetBody().GetWorldSpaceSurfaceNormal(joltResult.mSubShapeID2, ray.GetPointOnRay(joltResult.mFraction)));
                            }
                            break;
                        }
                    }
                }
                
            }

            return result;
        }
        void setEntityVelocity(entt::entity entity, const glm::vec3& velocity) {
            auto& reg = World::Get().Registry();
            if (auto* rb = reg.try_get<RigidBodyComponent>(entity)) {
                _manager.GetBodyInterface().SetLinearVelocity(rb->bodyID, ToJPH(velocity));
            }
		}

        RaycastHit SphereCast(const glm::vec3& origin, const glm::vec3& direction, float radius, float maxDistance, JPH::BodyID ignoreID = JPH::BodyID()) {
            RaycastHit result;

            if (maxDistance <= 0.0f || radius <= 0.0f) {
                return result;
            }

            JPH::SphereShape sphereShape(radius);
            sphereShape.SetEmbedded();

            JPH::RShapeCast shapeCast(
                &sphereShape,
                JPH::Vec3::sReplicate(1.0f),
                JPH::RMat44::sTranslation(ToJPHR(origin)),
                ToJPH(direction * maxDistance)
            );

            JPH::ShapeCastSettings settings;
            settings.mUseShrunkenShapeAndConvexRadius = true;
            settings.mReturnDeepestPoint = false;
            settings.mBackFaceModeTriangles = JPH::EBackFaceMode::IgnoreBackFaces;

            JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
            JPH::IgnoreSingleBodyFilter bodyFilter(ignoreID);

            _manager.GetSystem()->GetNarrowPhaseQuery().CastShape(
                shapeCast,
                settings,
                JPH::RVec3::sZero(),
                collector,
                {},
                {},
                bodyFilter
            );

            if (collector.HadHit()) {
                result.hasHit = true;
                result.distance = collector.mHit.mFraction * maxDistance;
                result.position = origin + direction * result.distance;
            }

            return result;
        }
    };
    class PhysicsSystem : public System {
    public:
        PhysicsSystem() = default;
        ~PhysicsSystem() override = default;

        Phase GetPhase() const override { return Phase::Physics; }

        void OnStart() override;
        void OnUpdate(float dt) override;
        void OnStop() override;


        void AddImpulse(entt::entity entity, const glm::vec3& impulse);

        void AddForce(entt::entity entity, const glm::vec3& force);

        RaycastHit Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, JPH::BodyID ignoreID = JPH::BodyID());
        void UpdateIsGrounded(entt::entity entity, float maxDistance = 0.1f);
    private:

        void OnRigidBodyCreated(entt::registry& reg, entt::entity entity);
        void OnRigidBodyDestroyed(entt::registry& reg, entt::entity entity);

    private:
        PhysicsManager _manager;
    };

} // namespace Kiki