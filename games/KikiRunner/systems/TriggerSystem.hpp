#pragma once
#include <kiki.h>

#include "Components/TriggerComponent.hpp"
#include "components/CharacterComponent.h"
#include "events/TriggerEvent.h"
#include "events/TeleportPerformedEvent.h"
#include "events/ResetLevelEvent.hpp"
#include "events/RequestLevelChangeEvent.hpp"
#include "events/RespawnCharacterEvent.hpp"

class TriggerSystem : public System {
public:
    Phase GetPhase() const override { return Phase::Update; }

    void OnStart() override {
        MessageCenter::Subscribe<ResetLevelEvent, &TriggerSystem::OnResetLevel>(this);
        MessageCenter::Subscribe<RequestLevelChangeEvent, &TriggerSystem::OnLevelChange>(this);
        MessageCenter::Subscribe<RespawnCharacterEvent, &TriggerSystem::OnRespawn>(this);
        MessageCenter::Subscribe<TeleportPerformedEvent, &TriggerSystem::OnTeleportPerformed>(this);
        ResolvePlayer();
    }

    void OnUpdate(float dt) override {
        ZoneScopedN("Trigger system update");

        if (playerEntity == NullEntity) {
            ResolvePlayer();
            if (playerEntity == NullEntity) return;
        }

        auto* pT = World::Get().GetComponent<TransformComponent>(playerEntity);
        if (!pT) return;

        glm::vec3 currPos = pT->position;
        if (!hasPrev) {
            prevPos = currPos;
            hasPrev = true;
            return;
        }

        constexpr float maxFrameMove = 5.0f;
        if (glm::distance(prevPos, currPos) > maxFrameMove) {
            prevPos = currPos;
            return;
        }

        auto view = World::Get().Query<TransformComponent, TriggerComponent>();
        for (auto [e, tT, trig] : view.each()) {
            if (!trig.active) continue;

            const bool ignoring = (e == ignoredTrigger);
            int dir = TestPlaneCrossing(tT, trig, prevPos, currPos);
            if (dir == 0) {
                if (ignoring && IsFarEnough(tT, trig, currPos)) {
                    ignoredTrigger = NullEntity;
                }
                continue;
            }
            if (ignoring) continue;

            MessageCenter::Publish(TriggerEvent{ e, playerEntity, dir });
            return;
        }

        prevPos = currPos;
    }

    void OnTeleportPerformed(const TeleportPerformedEvent& e) {
        if (e.actor != playerEntity) return;
        ignoredTrigger = e.dst;
        if (auto* pT = World::Get().GetComponent<TransformComponent>(playerEntity)) {
            prevPos = pT->position;
            hasPrev = true;
        }
    }

    void OnResetLevel(const ResetLevelEvent&) {
        ResetTracking();
        ResolvePlayer();
    }

    void OnLevelChange(const RequestLevelChangeEvent&) {
        playerEntity = NullEntity;
        ResetTracking();
    }

    void OnRespawn(const RespawnCharacterEvent&) {
        ResetTracking();
    }

private:
    Entity playerEntity = NullEntity;
    glm::vec3 prevPos = { 0.f, 0.f, 0.f };
    bool hasPrev = false;
    Entity ignoredTrigger = NullEntity;

    void ResetTracking() {
        hasPrev = false;
        ignoredTrigger = NullEntity;
    }

    void ResolvePlayer() {
        Entity prev = playerEntity;
        playerEntity = NullEntity;
        auto v = World::Get().Query<CharacterComponent>();
        for (auto [e, c] : v.each()) {
            playerEntity = e;
            break;
        }
        if (playerEntity != NullEntity && playerEntity != prev) {
            hasPrev = false;
            ignoredTrigger = NullEntity;
        }
    }

    static int TestPlaneCrossing(const TransformComponent& tT,
                                 const TriggerComponent& trig,
                                 const glm::vec3& a,
                                 const glm::vec3& b)
    {
        glm::vec3 normal = tT.rotation * glm::vec3(0.0f, 0.0f, -1.0f);
        float sa = glm::dot(a - tT.position, normal);
        float sb = glm::dot(b - tT.position, normal);
        if (sa * sb >= 0.0f) return 0;

        glm::vec3 right = tT.rotation * glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 up    = tT.rotation * glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 local = b - tT.position;
        if (std::abs(glm::dot(local, right)) > trig.halfExtents.x) return 0;
        if (std::abs(glm::dot(local, up))    > trig.halfExtents.y) return 0;

        return (sb > sa) ? +1 : -1;
    }

    static bool IsFarEnough(const TransformComponent& tT,
                            const TriggerComponent& trig,
                            const glm::vec3& p)
    {
        glm::vec3 normal = tT.rotation * glm::vec3(0.0f, 0.0f, -1.0f);
        float along = std::abs(glm::dot(p - tT.position, normal));
        float threshold = std::max(0.5f, trig.halfExtents.z + 0.25f);
        return along > threshold;
    }
};
