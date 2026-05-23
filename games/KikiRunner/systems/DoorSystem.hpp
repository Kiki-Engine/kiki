#pragma once
#include <kiki.h>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Components/DoorComponent.hpp"
#include "components/CharacterComponent.h"
#include "events/ResetLevelEvent.hpp"
#include "events/RequestLevelChangeEvent.hpp"
#include "events/LevelLoadedEvent.hpp"

class DoorSystem : public System {
public:
    Phase GetPhase() const override { return Phase::Update; }

    void OnStart() override {
        MessageCenter::Subscribe<LevelLoadedEvent,        &DoorSystem::OnLevelLoaded>(this);
        MessageCenter::Subscribe<ResetLevelEvent,         &DoorSystem::OnLevelReset>(this);
        MessageCenter::Subscribe<RequestLevelChangeEvent, &DoorSystem::OnLevelChange>(this);
        ResolvePlayer();
    }

    void OnUpdate(float dt) override {
        ZoneScopedN("Door system update");

        if (playerEntity == NullEntity) {
            ResolvePlayer();
            if (playerEntity == NullEntity) return;
        }
        auto* pT = World::Get().GetComponent<TransformComponent>(playerEntity);
        if (!pT) return;
        const glm::vec3 pPos = pT->position;

        auto view = World::Get().Query<TransformComponent, DoorComponent>();
        for (auto [e, tT, door] : view.each()) {
            const float openR  = door.triggerRadius;
            const float closeR = openR + 0.3f;
            const float openR2  = openR  * openR;
            const float closeR2 = closeR * closeR;

            const float d2 = DistSqXZ(pPos, tT.position);

            bool wantOpen;
            if (door.state == DoorComponent::State::Closed ||
                door.state == DoorComponent::State::Closing) {
                wantOpen = (d2 <= openR2);
            } else {
                wantOpen = (d2 <= closeR2);
            }

            if (wantOpen) {
                if (door.currentAngleDeg == 0.0f &&
                    door.state != DoorComponent::State::Opening &&
                    door.state != DoorComponent::State::Open) {
                    // Door surface normal in world space, projected to the
                    // horizontal plane so it works regardless of how the door
                    // was modeled / exported.
                    glm::vec3 fwd = door.closedRotation * glm::vec3(0.0f, 0.0f, 1.0f);
                    fwd.y = 0.0f;
                    glm::vec3 toPlayer = pPos - tT.position;
                    toPlayer.y = 0.0f;
                    float side = glm::dot(toPlayer, fwd);
                    door.openSign = (side >= 0.0f) ? 1.0f : -1.0f;
                }

                door.state = (door.currentAngleDeg < door.openAngleDeg)
                             ? DoorComponent::State::Opening
                             : DoorComponent::State::Open;
            } else {
                door.state = (door.currentAngleDeg > 0.0f)
                             ? DoorComponent::State::Closing
                             : DoorComponent::State::Closed;
            }

            const float step = door.speedDegPerSec * dt;
            if (door.state == DoorComponent::State::Opening) {
                door.currentAngleDeg = std::min(door.currentAngleDeg + step,
                                                door.openAngleDeg);
                if (door.currentAngleDeg >= door.openAngleDeg) {
                    door.state = DoorComponent::State::Open;
                }
            } else if (door.state == DoorComponent::State::Closing) {
                door.currentAngleDeg = std::max(door.currentAngleDeg - step, 0.0f);
                if (door.currentAngleDeg <= 0.0f) {
                    door.state = DoorComponent::State::Closed;
                }
            }

            // Rotate around WORLD up (Y) by left-multiplying the delta. This
            // guarantees the door always swings around a vertical axis through
            // its hinge, regardless of any tilt baked into closedRotation by
            // the exporter / parent hierarchy.
            glm::quat delta = glm::angleAxis(
                glm::radians(door.currentAngleDeg * door.openSign), door.axis);
            tT.rotation = delta * door.closedRotation;
            tT.dirty = true;
        }
    }

    void OnLevelLoaded(const LevelLoadedEvent&) {
        ResolvePlayer();
        CacheClosedRotation();
    }

    void OnLevelReset(const ResetLevelEvent&) {
        ResolvePlayer();
    }

    void OnLevelChange(const RequestLevelChangeEvent&) {
        playerEntity = NullEntity;
    }

private:
    Entity playerEntity = NullEntity;

    static float DistSqXZ(const glm::vec3& a, const glm::vec3& b) {
        const float dx = a.x - b.x;
        const float dz = a.z - b.z;
        return dx * dx + dz * dz;
    }

    void ResolvePlayer() {
        playerEntity = NullEntity;
        auto v = World::Get().Query<CharacterComponent>();
        for (auto [e, c] : v.each()) { playerEntity = e; break; }
    }

    void CacheClosedRotation() {
        auto view = World::Get().Query<TransformComponent, DoorComponent>();
        for (auto [e, tT, door] : view.each()) {
            if (door.currentAngleDeg == 0.0f) {
                door.closedRotation = tT.rotation;
            }
        }
    }
};
