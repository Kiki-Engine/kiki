#pragma once
#include <kiki.h>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <Jolt/Physics/Body/BodyInterface.h>

#include "Components/TriggerComponent.hpp"
#include "events/TriggerEvent.h"
#include "events/TeleportPerformedEvent.h"
#include "events/ResetLevelEvent.hpp"
#include "events/RequestLevelChangeEvent.hpp"
#include "events/LevelLoadedEvent.hpp"

class TeleportTriggerSystem : public System {
public:
    Phase GetPhase() const override { return Phase::Update; }

    void OnStart() override {
        MessageCenter::Subscribe<TriggerEvent, &TeleportTriggerSystem::OnTrigger>(this);
        MessageCenter::Subscribe<LevelLoadedEvent, &TeleportTriggerSystem::OnLevelLoaded>(this);
        MessageCenter::Subscribe<ResetLevelEvent, &TeleportTriggerSystem::OnResetLevel>(this);
        MessageCenter::Subscribe<RequestLevelChangeEvent, &TeleportTriggerSystem::OnLevelChange>(this);
        RebuildRings();
    }

    void OnUpdate(float dt) override {
    }

    void OnLevelLoaded(const LevelLoadedEvent&) {
        RebuildRings();
    }

    void OnResetLevel(const ResetLevelEvent&) {
        RebuildRings();
    }

    void OnLevelChange(const RequestLevelChangeEvent&) {
        rings.clear();
    }

    void OnTrigger(const TriggerEvent& e) {
        auto* tag = World::Get().GetComponent<TeleportTag>(e.trigger);
        if (!tag) return;

        auto it = rings.find(tag->loopNum);
        if (it == rings.end() || it->second.size() < 2) return;

        auto& list = it->second;
        int idx = -1;
        for (int i = 0; i < (int)list.size(); ++i) {
            if (list[i] == e.trigger) { idx = i; break; }
        }
        if (idx < 0) return;

        Entity dst = NullEntity;
        if (e.crossingDir > 0) {
            dst = list[(idx + 1) % list.size()];
        } else if (e.crossingDir < 0) {
            dst = list[(idx - 1 + list.size()) % list.size()];
        } else {
            return;
        }
        if (dst == NullEntity || dst == e.trigger) return;

        DoTeleport(e.actor, e.trigger, dst);
    }

private:
    std::unordered_map<int, std::vector<Entity>> rings;

    void RebuildRings() {
        rings.clear();
        auto v = World::Get().Query<TeleportTag>();
        for (auto [e, tag] : v.each()) {
            rings[tag.loopNum].push_back(e);
        }
        for (auto& [g, list] : rings) {
            std::sort(list.begin(), list.end(), [](Entity a, Entity b) {
                auto* ta = World::Get().GetComponent<TeleportTag>(a);
                auto* tb = World::Get().GetComponent<TeleportTag>(b);
                if (!ta || !tb) return false;
                return ta->order < tb->order;
            });
        }
    }

    void DoTeleport(Entity actor, Entity src, Entity dst) {
        auto& reg = World::Get().Registry();
        auto* aT = reg.try_get<TransformComponent>(actor);
        auto* sT = reg.try_get<TransformComponent>(src);
        auto* dT = reg.try_get<TransformComponent>(dst);
        if (!aT || !sT || !dT) return;

        glm::vec3 oldPos = aT->position;
        glm::quat oldRot = aT->rotation;

        glm::quat srcRotInv = glm::conjugate(sT->rotation);
        glm::quat deltaRot  = dT->rotation * srcRotInv;

        glm::vec3 newPos = dT->position + deltaRot * (oldPos - sT->position);
        glm::quat newRot = deltaRot * oldRot;

        glm::vec3 srcVel(0.0f);
        PhysicsService* physics = nullptr;
        if (reg.ctx().contains<PhysicsService>()) {
            physics = &reg.ctx().get<PhysicsService>();
        }
        RigidBodyComponent* rb = reg.try_get<RigidBodyComponent>(actor);
        if (physics && rb) {
            JPH::Vec3 v = physics->_manager.GetBodyInterface().GetLinearVelocity(rb->bodyID);
            srcVel = ToGLM(v);
        }
        glm::vec3 newVel = deltaRot * srcVel;

        aT->position = newPos;
        aT->rotation = newRot;
        aT->dirty = true;

        if (physics && rb) {
            physics->_manager.GetBodyInterface().SetPositionAndRotation(
                rb->bodyID,
                ToJPHR(newPos),
                ToJPH(newRot),
                JPH::EActivation::Activate);
            physics->setEntityVelocity(actor, newVel);
        }

        TeleportPerformedEvent ev;
        ev.actor = actor;
        ev.src = src;
        ev.dst = dst;
        ev.oldPosition = oldPos;
        ev.newPosition = newPos;
        ev.srcRotation = sT->rotation;
        ev.dstRotation = dT->rotation;
        ev.deltaRotation = deltaRot;
        ev.yawDeltaDegrees = ExtractYawDelta(deltaRot);
        MessageCenter::Publish(ev);
    }

    static float ExtractYawDelta(const glm::quat& q) {
        glm::vec3 forward = q * glm::vec3(0.0f, 0.0f, -1.0f);
        return glm::degrees(std::atan2(-forward.x, -forward.z));
    }
};
