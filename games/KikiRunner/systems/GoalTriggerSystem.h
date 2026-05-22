#pragma once
#include <kiki.h>
#include "Components/TriggerComponent.hpp"
#include "components/CharacterComponent.h"
#include "events/TimerTriggerEvent.h"
#include "events/TriggerEvent.h"
#include "events/ResetLevelEvent.hpp"
#include "events/RequestLevelChangeEvent.hpp"

class GoalTriggerSystem : public System {
public:
	Phase GetPhase() const override { return Phase::PostUpdate; }

	void OnUpdate(float dt) override {
		ZoneScopedN("Goal trigger system update");
		if (goalReached) {
			goalReached = false;
			if (playerEntity == NullEntity) ResolvePlayer();
			auto* playerTransform = World::Get().GetComponent<TransformComponent>(playerEntity);
			auto* cc = World::Get().GetComponent<CharacterComponent>(playerEntity);
			if (playerTransform && cc) {
				auto spawnPos = cc->spawnPosition;
				playerTransform->position = spawnPos;
				spdlog::info("Resetting player position to spawn point: x={}, y={}, z={}", spawnPos.x, spawnPos.y, spawnPos.z);
				playerTransform->dirty = true;
			}
		}
	}

	void OnStart() override {
		MessageCenter::Subscribe<CollisionEvent, &GoalTriggerSystem::OnCollision>(this);
		MessageCenter::Subscribe<TriggerEvent, &GoalTriggerSystem::OnTrigger>(this);
		MessageCenter::Subscribe<ResetLevelEvent, &GoalTriggerSystem::OnLevelReset>(this);
		MessageCenter::Subscribe<RequestLevelChangeEvent, &GoalTriggerSystem::OnLevelChange>(this);
		ResolvePlayer();
	}

	void OnLevelChange(const RequestLevelChangeEvent& e) {
		playerEntity = entt::null;
	}

	void OnLevelReset(const ResetLevelEvent& e) {
		ResolvePlayer();
	}

	void OnCollision(const CollisionEvent& e) {
		if (playerEntity == NullEntity) ResolvePlayer();
		if (playerEntity == NullEntity) return;

		Entity other = NullEntity;
		if (e.entity1 == playerEntity)      other = e.entity2;
		else if (e.entity2 == playerEntity) other = e.entity1;
		else return;

		if (!World::Get().Registry().any_of<GoalTag>(other)) return;

		FireGoal();
	}

	void OnTrigger(const TriggerEvent& e) {
		if (!World::Get().Registry().any_of<GoalTag>(e.trigger)) return;
		FireGoal();
	}

private:
	bool goalReached = false;
	Entity playerEntity = NullEntity;

	void ResolvePlayer() {
		playerEntity = NullEntity;
		auto v = World::Get().Query<CharacterComponent>();
		if (v.empty()) {
			spdlog::warn("No entities with CharacterComponent found when resolving player entity");
			return;
		}
		for (auto [e, c] : v.each()) {
			playerEntity = e;
			break;
		}
	}

	void FireGoal() {
		MessageCenter::Publish(TimerTriggerEvent{ Timer::get().Elapsed() });
		Timer::get().Reset();
		goalReached = true;
	}
};
