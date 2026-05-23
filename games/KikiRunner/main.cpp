#include <kiki.h>

#include "systems/UISystem.hpp"
#include "systems/LevelSystem.hpp"
#include "systems/CharacterSystem.h"
#include "systems/ThirdPersonCameraSystem.h"
#include "systems/GoalTriggerSystem.h"
#include "systems/TriggerSystem.hpp"
#include "systems/TeleportTriggerSystem.hpp"
#include "systems/DoorSystem.hpp"
#include "systems/TimeLimitSystem.h"

int main(int argc, char** argv) {
	Kiki::Engine engine;
	engine.Init();


	// example of setting a custom skybox
	Kiki::RenderManager::get().setCustomSkybox(
		std::filesystem::path(PROJECT_ROOT_PATH) / "assets/custom_skybox_right.png",
		std::filesystem::path(PROJECT_ROOT_PATH) / "assets/custom_skybox_left.png",
		std::filesystem::path(PROJECT_ROOT_PATH) / "assets/custom_skybox_up.png",
		std::filesystem::path(PROJECT_ROOT_PATH) / "assets/custom_skybox_down.png",
		std::filesystem::path(PROJECT_ROOT_PATH) / "assets/custom_skybox_front.png",
		std::filesystem::path(PROJECT_ROOT_PATH) / "assets/custom_skybox_back.png"
	);

	engine.RegisterSystem<LevelSystem>();
	engine.RegisterSystem<CharacterSystem>();
	engine.RegisterSystem<ThirdPersonCameraSystem>();
	engine.RegisterSystem<TriggerSystem>();
	engine.RegisterSystem<TeleportTriggerSystem>();
	engine.RegisterSystem<GoalTriggerSystem>();
	engine.RegisterSystem<DoorSystem>();
	engine.RegisterSystem<TimeLimitSystem>();
	engine.RegisterSystem<UISystem>();

	engine.Run();
}