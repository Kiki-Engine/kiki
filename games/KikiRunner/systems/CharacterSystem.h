#pragma once
#include <kiki.h>
#include "components/CharacterComponent.h"
#include "components/ThirdPersonCameraComponent.hpp"
#include "Animation/AnimationComponent.h"
#include "events/TimerTriggerEvent.h"
#include "events/ResetLevelEvent.hpp"
#include "events/ResetThirdPersonCameraEvent.hpp"
#include "events/ObjectiveAchievedEvent.hpp"
#include "events/RequestLevelChangeEvent.hpp"
#include "events/RespawnCharacterEvent.hpp"
#include "events/TeleportPerformedEvent.h"
#include "../../../kiki-engine/Audio/BGMController.h"
class CharacterSystem : public System {
public:
    Phase GetPhase() const override { return Phase::Update; }

    void OnUpdate(float dt) override {
        
		ZoneScopedN("Character system update");

        auto objects = World::Get().Query<TransformComponent, CharacterComponent,PhysicalAttributesComponent>();
		for (auto [entity, transform, character,ip] : objects.each()) {
            if (character.jumpTimer > 0.0f) {
                character.jumpTimer -= dt;
            }
			if (dashTimer > 0.0f) {
				dashTimer -= dt;
            }
            float cameraYaw = GetCameraYaw(entity);
			HandleMovement(transform, character, ip, cameraYaw, dt);
			HandleJump(entity, transform, character, ip, dt);
			HandleRotation(transform, character, dt);
            UpdateState(character,ip);

		    if (transform.position.y < -25.0f) {
		        MessageCenter::Publish(RespawnCharacterEvent());
		    }
        }
        auto& inputManager = Kiki::InputManager::get();
    }
    void OnStart() override {
        Reset();
        MessageCenter::Subscribe<TimerTriggerEvent, &CharacterSystem::OnTimerTrigger>(this);
        MessageCenter::Subscribe<ResetLevelEvent, &CharacterSystem::OnResetEvent>(this);
        MessageCenter::Subscribe<RequestLevelChangeEvent, &CharacterSystem::OnLevelChange>(this);
        MessageCenter::Subscribe<RespawnCharacterEvent, &CharacterSystem::OnRespawnEvent>(this);
        MessageCenter::Subscribe<TeleportPerformedEvent, &CharacterSystem::OnTeleportPerformed>(this);
    }

    void OnTeleportPerformed(const TeleportPerformedEvent& e) {
        if (e.actor != playerEntity) return;
        auto* character = World::Get().GetComponent<CharacterComponent>(playerEntity);
        auto* transform = World::Get().GetComponent<TransformComponent>(playerEntity);
        if (!character || !transform) return;

        glm::vec3 fwd = transform->rotation * glm::vec3(0.0f, 0.0f, 1.0f);
        float yawDeg = glm::degrees(std::atan2(-fwd.x, -fwd.z));
        character->facingYaw = yawDeg;
        character->targetYaw = yawDeg;

        glm::vec3 vel = e.deltaRotation * character->velocity;
        character->velocity = vel;
    }
    void OnStop() override {
       
    }

    void OnResetEvent(const ResetLevelEvent& e) {
        Reset();
        MessageCenter::Publish(ResetThirdPersonCameraEvent());
    }

    void OnRespawnEvent(const RespawnCharacterEvent& e) {
        Timer::get().Reset();
        auto objects2 = World::Get().Query<MiscComponent>();
        glm::vec3 spawnPos = glm::vec3(0, 0, 0);
        //Entity playerEntity = NullEntity;
        for (auto [e, misc] : objects2.each()) {
            if (misc.miscTag == MmiscTags::SPAWN) {
                auto* transform = World::Get().GetComponent<TransformComponent>(e);
                if (transform) {
                    spawnPos = transform->position;
                }
            }
        }

        if (playerEntity != NullEntity) {
            auto* character = World::Get().GetComponent<CharacterComponent>(playerEntity);
            auto* transform = World::Get().GetComponent<TransformComponent>(playerEntity);
            auto* physics = World::Get().GetComponent<PhysicalAttributesComponent>(playerEntity);
            if (character) {
                character->spawnPosition = spawnPos;
            }
            if (transform) {
                transform->position = spawnPos;
                float modelRotationOffset = 180.0f;
                transform->rotation = glm::angleAxis(
                    glm::radians(character->facingYaw + modelRotationOffset),
                    glm::vec3(0, 1, 0)
                );
                transform->dirty = true;
            }
            if (physics) {
                physics->isGroundedNeedsUpdate = true;
            }
        }
    }

    void OnLevelChange(const RequestLevelChangeEvent& e) {
        playerEntity = entt::null;
    }
private:
    InputManager& inputManager = Kiki::InputManager::get();
	Entity playerEntity = NullEntity;
	bool isDoubleJumping = false;
	float dashTimer = 0.0f;// for dash ability cooldown
    float GetCameraYaw(Entity targetEntity) {
        float yaw = 0.0f;
        auto camView = World::Get().Query<ThirdPersonCameraComponent>();
        for (auto [e, cam] : camView.each()) {
            if (cam.followTarget == targetEntity) {
                yaw = cam.yaw;
                break;
            }
        }
        return yaw;
    }

    void Reset() {
        auto objects2 = World::Get().Query<MiscComponent>();
        glm::vec3 spawnPos = glm::vec3(0, 0, 0);
        //Entity playerEntity = NullEntity;
        for (auto [e, misc] : objects2.each()) {
            if (misc.miscTag == MmiscTags::PLAYER) {
                playerEntity = e;
                World::Get().Registry().emplace<CharacterComponent>(e);
            }
            if (misc.miscTag == MmiscTags::SPAWN) {
                auto* transform = World::Get().GetComponent<TransformComponent>(e);
                if (transform) {
                    spawnPos = transform->position;
                }
            }
        }

        if (playerEntity != NullEntity) {
            auto* character = World::Get().GetComponent<CharacterComponent>(playerEntity);
            auto* transform = World::Get().GetComponent<TransformComponent>(playerEntity);
            auto* physics = World::Get().GetComponent<PhysicalAttributesComponent>(playerEntity);
            if (character) {
                character->spawnPosition = spawnPos;
            }
            if (transform) {
                transform->position = spawnPos;
                float modelRotationOffset = 180.0f;
                transform->rotation = glm::angleAxis(
                    glm::radians(character->facingYaw + modelRotationOffset),
                    glm::vec3(0, 1, 0)
                );
                transform->dirty = true;
            }
            if (physics) {
                physics->isGroundedNeedsUpdate = true;
            }
        }
    }

    void HandleMovement(
        TransformComponent& transform,
        CharacterComponent& character,
        PhysicalAttributesComponent& ip,
        float cameraYaw, float dt)
    {
        glm::vec2 inputDir = { 0, 0 };
        bool usingKeyboard = false; 

        if (inputManager.isKeyDown(GLFW_KEY_W)) { inputDir.y += 1.0f; usingKeyboard = true; }
        if (inputManager.isKeyDown(GLFW_KEY_S)) { inputDir.y -= 1.0f; usingKeyboard = true; }
        if (inputManager.isKeyDown(GLFW_KEY_A)) { inputDir.x -= 1.0f; usingKeyboard = true; }
        if (inputManager.isKeyDown(GLFW_KEY_D)) { inputDir.x += 1.0f; usingKeyboard = true; }

        float stickX = inputManager.getGamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_X);
        float stickY = -inputManager.getGamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_Y);

        const float deadzone = 0.15f;
        if (std::abs(stickX) > deadzone) inputDir.x += stickX;
        if (std::abs(stickY) > deadzone) inputDir.y += stickY;

        float inputMag = glm::length(inputDir);
        if (inputMag > 1.0f) {
            inputDir /= inputMag;
            inputMag = 1.0f;
        }

        float currentMoveSpeed = 0.0f;

        if (ip.isGrounded) {
            if (usingKeyboard) {
                bool isRunning = inputManager.isKeyDown(GLFW_KEY_LEFT_SHIFT);
                character.currentMaxSpeed = isRunning ? character.runSpeed : character.walkSpeed;
                currentMoveSpeed = character.currentMaxSpeed;
            }
            else {
				const float runThreshold = 0.85f; //here change the threshold for stick input to trigger running
                if (inputMag >= runThreshold) {
                    character.currentMaxSpeed = character.runSpeed;
                    currentMoveSpeed = character.runSpeed; 
                }
                else {
                    character.currentMaxSpeed = character.walkSpeed;
                    float walkRatio = inputMag / runThreshold;
                    currentMoveSpeed = character.walkSpeed * walkRatio; 
                }
            }
        }
        else {
            currentMoveSpeed = character.currentMaxSpeed;
        }

        float maxDashSpeed = character.currentMaxSpeed; 
        if (character.hasAbility(Ability::SpeedBoost)) {
            currentMoveSpeed *= 2.0f;
            maxDashSpeed *= 2.0f;
        }

        PhysicsService& physics = World::Get().Registry().ctx().get<PhysicsService>();

        auto* rb = World::Get().GetComponent<RigidBodyComponent>(playerEntity);
        JPH::Vec3 currentJoltVel = physics._manager.GetBodyInterface().GetLinearVelocity(rb->bodyID);

        bool hasInput = inputMag > 0.001f;
        glm::vec3 moveDir(0.0f);
        glm::vec3 targetHorizVel(0.0f);

        if (hasInput) {
            glm::vec2 normInputDir = inputDir / inputMag;

            float rad = glm::radians(cameraYaw);
            glm::vec3 forward = { -sin(rad), 0, -cos(rad) };
            glm::vec3 right = { cos(rad), 0, -sin(rad) };

            moveDir = forward * normInputDir.y + right * normInputDir.x;
            targetHorizVel = moveDir * currentMoveSpeed;

            character.targetYaw = glm::degrees(atan2(-moveDir.x, -moveDir.z));
        }

        bool isDashPressed = inputManager.isMouseButtonDown(GLFW_MOUSE_BUTTON_2) ||
            inputManager.isGamepadButtonDown(GLFW_GAMEPAD_BUTTON_X);
        bool wantsDash = hasInput && character.hasAbility(Ability::Dash) && isDashPressed && dashTimer <= 0.0f;

        if (ip.isGrounded) {
            glm::vec3 currentHorizVel(character.velocity.x, 0.0f, character.velocity.z);
            float smoothRate = hasInput ? character.acceleration : character.deceleration;
            float lerpFactor = 1.0f - std::exp(-smoothRate * dt);
            glm::vec3 newHorizVel = glm::mix(currentHorizVel, targetHorizVel, lerpFactor);

            character.velocity.x = newHorizVel.x;
            character.velocity.z = newHorizVel.z;

            glm::vec3 finalVel = character.velocity + ip.PointVelocity;
            glm::vec3 newVel = glm::vec3(finalVel.x, currentJoltVel.GetY(), finalVel.z);

            if (wantsDash) {
                newVel += moveDir * maxDashSpeed * 100.0f;
                dashTimer = 1.0f;
            }
            physics.setEntityVelocity(playerEntity, newVel);
        }
        else {
            if (hasInput) {
                glm::vec3 currentHorizVel(character.velocity.x, 0.0f, character.velocity.z);
                float lerpFactor = 1.0f - std::exp(-character.airControl * dt);
                glm::vec3 newHorizVel = glm::mix(currentHorizVel, targetHorizVel, lerpFactor);

                character.velocity.x = newHorizVel.x;
                character.velocity.z = newHorizVel.z;

                glm::vec3 finalVel = character.velocity + ip.PointVelocity;
                glm::vec3 newVel = glm::vec3(finalVel.x, currentJoltVel.GetY(), finalVel.z);

                if (wantsDash) {
                    newVel += moveDir * maxDashSpeed * 100.0f;
                    dashTimer = 1.0f;
                }
                physics.setEntityVelocity(playerEntity, newVel);
            }
            else {
                character.velocity.x = currentJoltVel.GetX();
                character.velocity.z = currentJoltVel.GetZ();
            }
        }

        transform.dirty = true;
    }
    void HandleJump(
        Entity entity,
        TransformComponent& transform,
        CharacterComponent& character,
        PhysicalAttributesComponent& ip,
        float dt)
    {
        if (inputManager.isKeyJustDown(GLFW_KEY_SPACE) || inputManager.isGamepadButtonJustDown(GLFW_GAMEPAD_BUTTON_A)) {
            if (character.state != Kiki::CharacterState::Jumping) {
                PhysicsService& physics = World::Get().Registry().ctx().get<PhysicsService>();
                auto* rb = World::Get().GetComponent<RigidBodyComponent>(entity);
                JPH::Vec3 currentVel = physics._manager.GetBodyInterface().GetLinearVelocity(rb->bodyID);

                //AUDIO TEST!!!
                //Kiki::AudioSystem::PlayOneShot("sounds/cao.mp3");

                physics._manager.GetBodyInterface().SetLinearVelocity(rb->bodyID, JPH::Vec3(currentVel.GetX(), character.jumpForce, currentVel.GetZ()));

                character.state = Kiki::CharacterState::Jumping;
                character.jumpTimer = 0.15f;
            }
            else if (character.hasAbility(Ability::DoubleJump) && !isDoubleJumping) {
                PhysicsService& physics = World::Get().Registry().ctx().get<PhysicsService>();
                auto* rb = World::Get().GetComponent<RigidBodyComponent>(entity);
                JPH::Vec3 currentVel = physics._manager.GetBodyInterface().GetLinearVelocity(rb->bodyID);
                physics._manager.GetBodyInterface().SetLinearVelocity(rb->bodyID, JPH::Vec3(currentVel.GetX(), character.jumpForce, currentVel.GetZ()));
                character.state = Kiki::CharacterState::Jumping;
                character.jumpTimer = 0.15f;
                isDoubleJumping = true; // prevent further double jumps until grounded again
            }
        }
    }
	// smoothly rotate character to face movement direction
    void HandleRotation(TransformComponent& transform,
        CharacterComponent& character,
        float dt)
    {
		// keep current facing direction if not moving
        if (glm::length(glm::vec2(character.velocity.x,
            character.velocity.z)) < 0.001f)
            return;

		// interpolate facingYaw towards targetYaw
        float diff = character.targetYaw - character.facingYaw;

		// handle angle wrap-around (e.g. from 350 to 10 degrees should rotate 20 degrees, not -340)
        if (diff > 180.0f)  diff -= 360.0f;
        if (diff < -180.0f) diff += 360.0f;

        character.facingYaw += diff * character.rotateSpeed * dt;
        float modelRotationOffset = 180.0f;

		// update transform rotation to match facing direction
        transform.rotation = glm::angleAxis(
            glm::radians(character.facingYaw + modelRotationOffset),
            glm::vec3(0, 1, 0)
        );
        transform.dirty = true;
    }
    void UpdateState(CharacterComponent& character, PhysicalAttributesComponent& pa) {
        bool isMoving = glm::length(glm::vec2(character.velocity.x, character.velocity.z)) > 0.1f;
        float currentSpeed = glm::length(glm::vec2(character.velocity.x, character.velocity.z));

        PhysicsService& physics = World::Get().Registry().ctx().get<PhysicsService>();
        auto* rb = World::Get().GetComponent<RigidBodyComponent>(playerEntity);
        float realVy = physics._manager.GetBodyInterface().GetLinearVelocity(rb->bodyID).GetY();

        if (character.state == Kiki::CharacterState::Jumping) {
            if (character.jumpTimer <= 0.0f && pa.isGrounded) {
                character.state = isMoving ? Kiki::CharacterState::Walking : Kiki::CharacterState::Idle;
                isDoubleJumping = false; // reset double jump when grounded
            }
        }
        else {
            if (isMoving) {
                if (currentSpeed > character.walkSpeed + 1.0f) {
                    character.state = Kiki::CharacterState::Running;
                }
                else {
                    character.state = Kiki::CharacterState::Walking;
                }
            }
            else {
                character.state = Kiki::CharacterState::Idle;
            }
        }

        Kiki::AnimationComponent* animComp = World::Get().GetComponent<Kiki::AnimationComponent>(playerEntity);
        if (animComp) {
            bool shouldLoop = (character.state != Kiki::CharacterState::Jumping);

            if (animComp->currentState != character.state) {
                animComp->ChangeState(character.state, shouldLoop);
            }
        }
    }
     void OnTimerTrigger(const TimerTriggerEvent& e) {
        auto* cc = World::Get().GetComponent<CharacterComponent>(playerEntity);
        std::vector<float>& timelimits = cc->timeLimits;
        float elapsed = e.elapsedTime;
        for (size_t i = 0; i < timelimits.size(); ++i) {
			if (cc->isDone[i]) continue; // skip already completed tiers

            if (timelimits[i] > elapsed) {
                MessageCenter::Publish(ObjectiveAchievedEvent(i));
                cc->isDone[i] = true;
                spdlog::info("You completed the level in {:.2f} seconds! You earned a new ability!", elapsed);
                if (i == 0) {
                    cc->grantAbility(Ability::DoubleJump);
					spdlog::info("You earned the double jump ability!");
                    //BGM TEST!!!
                    Kiki::BGMController::get().Play("interface/success.mp3");
                }
                else if (i == 1) {
                    cc->grantAbility(Ability::SpeedBoost);
					spdlog::info("You earned the speed boost ability!");
                }
                else if (i == 2) {
                    cc->grantAbility(Ability::Dash);
					spdlog::info("You earned the dash ability!");
                }
                else if (i == 3) {
					// completed level logic here
					spdlog::info("Congratulations! You completed the level! ");
					//BGM TEST!!!
                    Kiki::BGMController::get().Stop();
                    
                }
                break;
            }
        }
    }
};