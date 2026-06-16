#ifndef KIKIRUNNER_LEVELSYSTEM
#define KIKIRUNNER_LEVELSYSTEM

#include <kiki.h>

#include "events/RequestLevelChangeEvent.hpp"
#include "events/LevelLoadedEvent.hpp"

class LevelSystem : public System {
	public:
	Phase GetPhase() const override { return Phase::PreUpdate; }

	void OnStart() override {
		MessageCenter::Subscribe<RequestLevelChangeEvent, &LevelSystem::OnTriggerEnter>(this);
	}

	void OnUpdate(float dt) override {
		ZoneScopedN("Level system update");
		if (loaded) {
			loaded = false;
			MessageCenter::Publish(LevelLoadedEvent());
		}
	}

	void OnTriggerEnter(const RequestLevelChangeEvent& e) {
		std::thread([this, e]() {
			sceneManager.clearLevel();
			for (auto path : e.levelPaths) {
				sceneManager.loadScene(Kiki::GltfLoaderAssimp::loadScene(path));
			}
			loaded = true;
		}).detach();
	}

	private:
	SceneManager& sceneManager = Kiki::SceneManager::get();
	bool loaded = false;
};

#endif