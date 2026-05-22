#pragma once
#include <kiki.h>
#include "components/CharacterComponent.h"
#include "components/ThirdPersonCameraComponent.hpp"
#include "events/ResetThirdPersonCameraEvent.hpp"
#include "events/TeleportPerformedEvent.h"

class ThirdPersonCamera : public Camera {
public:
	ThirdPersonCamera() : Camera() {
		auto& reg = World::Get().Registry();
		reg.emplace<ThirdPersonCameraComponent>(camera);
	}

};
class ThirdPersonCameraSystem : public System {
public:
	Phase GetPhase()    const override { return Phase::Update; }
	void OnUpdate(float dt) override {
		//if (inputManager.isKeyJustDown(GLFW_KEY_SPACE) && !inputManager.isCursorDisabledFunc()) {
		//	inputManager.disableCursor();
		//	//spdlog::info("Right mouse button down, cursor disabled");
		//}
		ZoneScopedN("Third person camera update");
		auto objects = World::Get().Query<TransformComponent, ThirdPersonCameraComponent, CameraComponent>();
		for (auto [entity, transform, tpscam,cam] : objects.each()) {
			HandleCameraRotation( tpscam,transform, dt);
			HandleCameraZoom(tpscam);
			HandleCameraPosition(transform, cam, tpscam, dt);
		
		}
	}
	void OnStart() override {
		Reset();
		MessageCenter::Subscribe<ResetThirdPersonCameraEvent, &ThirdPersonCameraSystem::OnResetEvent>(this);
		MessageCenter::Subscribe<TeleportPerformedEvent, &ThirdPersonCameraSystem::OnTeleportPerformed>(this);
	}

	void OnResetEvent(const ResetThirdPersonCameraEvent& e) {
		Reset();
	}

	void OnTeleportPerformed(const TeleportPerformedEvent& e) {
		auto camView = World::Get().Query<TransformComponent, ThirdPersonCameraComponent>();
		for (auto [ce, camTransform, cam] : camView.each()) {
			if (cam.followTarget != e.actor) continue;

			glm::vec3 oldPivot = e.oldPosition + glm::vec3(0, cam.height, 0);
			glm::vec3 newPivot = e.newPosition + glm::vec3(0, cam.height, 0);

			if (cam.currentPosInitialized) {
				cam.currentPos = newPivot + e.deltaRotation * (cam.currentPos - oldPivot);
			} else {
				cam.currentPos = newPivot;
				cam.currentPosInitialized = true;
			}

			camTransform.position = newPivot + e.deltaRotation * (camTransform.position - oldPivot);
			camTransform.rotation = e.deltaRotation * camTransform.rotation;
			camTransform.dirty = true;

			cam.yaw += e.yawDeltaDegrees;
		}
	}

	static void SnapCameraToTarget(TransformComponent& camTransform, ThirdPersonCameraComponent& cam) {
		auto* targetTransform = World::Get().GetComponent<TransformComponent>(cam.followTarget);
		if (!targetTransform) return;

		glm::vec3 pivotPos = targetTransform->position + glm::vec3(0, cam.height, 0);
		float yawRad   = glm::radians(cam.yaw);
		float pitchRad = glm::radians(cam.pitch);
		glm::vec3 offset = {
			cam.distance * cos(pitchRad) * sin(yawRad),
			cam.distance * sin(pitchRad),
			cam.distance * cos(pitchRad) * cos(yawRad)
		};
		glm::vec3 dir = glm::normalize(offset);

		cam.currentDistance = cam.distance;
		cam.currentPos = pivotPos + dir * cam.distance;
		cam.currentPosInitialized = true;

		camTransform.position = cam.currentPos;
		camTransform.dirty = true;
		glm::vec3 forward = glm::normalize(pivotPos - camTransform.position);
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		camTransform.rotation = glm::quatLookAt(forward, up);
	}
	
private:
	InputManager& inputManager = Kiki::InputManager::get();
	ThirdPersonCamera camera;
	bool isDisablingCursor = false;
	void HandleCameraRotation(ThirdPersonCameraComponent& cam, TransformComponent& trans, float dt)
	{
		float yawInput = 0.0f;
		float pitchInput = 0.0f;

		float mousedX = 0.0f, mousedY = 0.0f;
		if (inputManager.isCursorDisabledFunc()) {
			inputManager.getMouseDeltaPosition(mousedX, mousedY);
			yawInput += mousedX * cam.rotateSensitivity;
			pitchInput += mousedY * cam.rotateSensitivity;
		}

		float stickX = inputManager.getGamepadAxis(GLFW_GAMEPAD_AXIS_RIGHT_X);
		float stickY = inputManager.getGamepadAxis(GLFW_GAMEPAD_AXIS_RIGHT_Y);

		const float deadzone = 0.15f;
		if (std::abs(stickX) > deadzone || std::abs(stickY) > deadzone) {

			float curveX = stickX * stickX * stickX;
			float curveY = stickY * stickY * stickY;

			float gamepadSensitivityX = 180.0f;
			float gamepadSensitivityY = 120.0f;

			yawInput += curveX * gamepadSensitivityX * dt;
			pitchInput += curveY * gamepadSensitivityY * dt;
		}

		if (std::abs(yawInput) < 0.001f && std::abs(pitchInput) < 0.001f) return;

		cam.yaw -= yawInput;
		cam.pitch += pitchInput;

		cam.pitch = glm::clamp(cam.pitch, cam.minPitch, cam.maxPitch);
	}

	void Reset() {
		auto objects = World::Get().Query<CharacterComponent, PhysicalAttributesComponent>();

		if (auto tpcc = World::Get().GetComponent<ThirdPersonCameraComponent>(camera.camera)) {
			for (auto [entity, chara, phys] : objects.each()) {
				tpcc->followTarget = entity;
				spdlog::info("Camera follow target locked to entity ID: {}", (uint32_t)entity);
				break;
			}
		}
		World::Get().GetComponent<CameraComponent>(camera.camera)->isMain = true;
	}

	void HandleCameraZoom(ThirdPersonCameraComponent& cam) {
		float scroll = inputManager.getMouseScrollDelta();
		if (scroll == 0.0f) return;
		cam.distance -= scroll * cam.zoomSensitivity;
		cam.distance = glm::clamp(cam.distance, cam.minDistance, cam.maxDistance);
	}

	void HandleCameraPosition(TransformComponent& camTransform,
		CameraComponent& camera,
		ThirdPersonCameraComponent& tpsCam,
		float dt)
	{
		if (tpsCam.followTarget == NullEntity) return;

		auto* targetTransform = World::Get().GetComponent<TransformComponent>(tpsCam.followTarget);
		if (!targetTransform) return;

		glm::vec3 pivotPos = targetTransform->position
			+ glm::vec3(0, tpsCam.height, 0);

		float yawRad = glm::radians(tpsCam.yaw);
		float pitchRad = glm::radians(tpsCam.pitch);

		glm::vec3 offset = {
			tpsCam.distance * cos(pitchRad) * sin(yawRad),
			tpsCam.distance * sin(pitchRad),
			tpsCam.distance * cos(pitchRad) * cos(yawRad)
		};

		glm::vec3 dir = glm::normalize(offset);

		PhysicsService& physics = World::Get().Registry().ctx().get<PhysicsService>();
		JPH::BodyID ignoreID;
		if (auto* rb = World::Get().GetComponent<RigidBodyComponent>(tpsCam.followTarget)) {
			ignoreID = rb->bodyID;
		}

		float requestedDistance = tpsCam.distance;
		auto hit = physics.SphereCast(pivotPos, dir, tpsCam.collisionRadius, requestedDistance + tpsCam.collisionBuffer, ignoreID);
		float targetDistance = requestedDistance;
		if (hit.hasHit) {
			targetDistance = std::max(tpsCam.minCollisionDistance, hit.distance - tpsCam.collisionBuffer);
		}

		if (!tpsCam.currentPosInitialized) {
			tpsCam.currentDistance = targetDistance;
			tpsCam.currentPos = pivotPos + dir * targetDistance;
			tpsCam.currentPosInitialized = true;
		}

		float smoothRate = (targetDistance < tpsCam.currentDistance)
			? tpsCam.pullInSpeed
			: tpsCam.pushOutSpeed;
		float distLerp = 1.0f - std::exp(-smoothRate * dt);
		tpsCam.currentDistance = glm::mix(tpsCam.currentDistance, targetDistance, distLerp);

		glm::vec3 desiredPos = pivotPos + dir * tpsCam.currentDistance;

		float posLerp = 1.0f - std::exp(-tpsCam.smoothSpeed * dt);
		tpsCam.currentPos = glm::mix(tpsCam.currentPos, desiredPos, posLerp);

		glm::vec3 toCam = tpsCam.currentPos - pivotPos;
		float toCamLen = glm::length(toCam);
		if (toCamLen > 0.001f) {
			glm::vec3 toCamDir = toCam / toCamLen;
			auto safety = physics.SphereCast(pivotPos, toCamDir, tpsCam.collisionRadius, toCamLen + tpsCam.collisionBuffer, ignoreID);
			if (safety.hasHit) {
				float safeDist = std::max(tpsCam.minCollisionDistance, safety.distance - tpsCam.collisionBuffer);
				if (safeDist < toCamLen) {
					tpsCam.currentPos = pivotPos + toCamDir * safeDist;
					tpsCam.currentDistance = std::min(tpsCam.currentDistance, safeDist);
				}
			}
		}

		camTransform.position = tpsCam.currentPos;
		camTransform.dirty = true;
		glm::vec3 forward = glm::normalize(pivotPos - camTransform.position);
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

		camTransform.rotation = glm::quatLookAt(forward, up);
	}
};