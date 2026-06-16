#ifndef KIKIRUNNER_UISYSTEM
#define KIKIRUNNER_UISYSTEM

#include <kiki.h>

#include "events/LevelLoadedEvent.hpp"
#include "events/RequestLevelChangeEvent.hpp"
#include "events/ResetLevelEvent.hpp"
#include "events/TimerTriggerEvent.h"
#include "events/ObjectiveAchievedEvent.hpp"
#include "events/RespawnCharacterEvent.hpp"
#include "components/CharacterComponent.h"

enum class ScreenType {
	SPLASH,
	LOADING,
	MAIN_MENU,
	LEVEL
};

struct ScreenBase {
	virtual ~ScreenBase() {};
};

struct SplashScreen : ScreenBase {
	entt::entity background = entt::null;
	entt::entity engineLogo = entt::null;
	entt::entity gameLogo = entt::null;
	entt::entity spinner = entt::null;

	SplashScreen() = default;
	~SplashScreen() {
		World& world = World::Get();

		{
			std::lock_guard<std::mutex> lock(Kiki::SceneManager::get().registryMutex);
			world.DestroyEntity(background);
			world.DestroyEntity(engineLogo);
			world.DestroyEntity(gameLogo);
			world.DestroyEntity(spinner);
		}
	};
};

struct LoadingScreen : ScreenBase {
	entt::entity background = entt::null;
	entt::entity spinner = entt::null;

	LoadingScreen() = default;
	~LoadingScreen() {
		World& world = World::Get();

		{
			std::lock_guard<std::mutex> lock(Kiki::SceneManager::get().registryMutex);
			world.DestroyEntity(background);
			world.DestroyEntity(spinner);
		}
	};
};

struct MainMenuScreen : ScreenBase {
	Camera* camera = nullptr;

	entt::entity middleContainer = entt::null;
	entt::entity gameLogo = entt::null;
	entt::entity background = entt::null;
	entt::entity playButton = entt::null;
	entt::entity playButtonInner = entt::null;
	entt::entity play2Button = entt::null;
	entt::entity play2ButtonInner = entt::null;
	entt::entity quitGameButton = entt::null;
	entt::entity quitGameButtonInner = entt::null;

	MainMenuScreen() = default;
	~MainMenuScreen() {
		World& world = World::Get();

		{
			std::lock_guard<std::mutex> lock(Kiki::SceneManager::get().registryMutex);
			world.DestroyEntity(quitGameButtonInner);
			world.DestroyEntity(quitGameButton);
			world.DestroyEntity(playButtonInner);
			world.DestroyEntity(playButton);
			world.DestroyEntity(play2ButtonInner);
			world.DestroyEntity(play2Button);
			world.DestroyEntity(background);
			world.DestroyEntity(gameLogo);
			world.DestroyEntity(middleContainer);
			delete camera;
		}
	};
};

struct Objective {
	entt::entity time;
	entt::entity powerup;
	entt::entity strikethrough;
	entt::entity tick;

	bool complete = false;
};

struct LevelScreen : ScreenBase {
	entt::entity timerBackground;
	entt::entity timer;

	entt::entity objectivesContainer;
	entt::entity objectivesBackground;
	entt::entity objectivesTitle;
	entt::entity objectivesTitleDivider;
	entt::entity objectivesTimeLimit;
	entt::entity objectivesPowerup;
	entt::entity objectivesSubtitleDivider;

	std::vector<Objective> objectives;

	bool paused = false;

	entt::entity middleContainer = entt::null;
	entt::entity background = entt::null;
	entt::entity playButton = entt::null;
	entt::entity playButtonInner = entt::null;
	entt::entity respawnButton = entt::null;
	entt::entity respawnButtonInner = entt::null;
	entt::entity menuButton = entt::null;
	entt::entity menuButtonInner = entt::null;
	entt::entity quitGameButton = entt::null;
	entt::entity quitGameButtonInner = entt::null;

	LevelScreen() = default;
	~LevelScreen() {
		World& world = World::Get();

		{
			std::lock_guard<std::mutex> lock(Kiki::SceneManager::get().registryMutex);
			world.DestroyEntity(timer);
			world.DestroyEntity(timerBackground);

			for (auto obj : objectives) {
				world.DestroyEntity(obj.time);
				world.DestroyEntity(obj.powerup);
				world.DestroyEntity(obj.strikethrough);
				world.DestroyEntity(obj.tick);
			}
			objectives.clear();

			world.DestroyEntity(objectivesSubtitleDivider);
			world.DestroyEntity(objectivesPowerup);
			world.DestroyEntity(objectivesTimeLimit);
			world.DestroyEntity(objectivesTitleDivider);
			world.DestroyEntity(objectivesTitle);
			world.DestroyEntity(objectivesBackground);
			world.DestroyEntity(objectivesContainer);

			world.DestroyEntity(quitGameButtonInner);
			world.DestroyEntity(quitGameButton);
			world.DestroyEntity(menuButtonInner);
			world.DestroyEntity(menuButton);
			world.DestroyEntity(respawnButtonInner);
			world.DestroyEntity(respawnButton);
			world.DestroyEntity(playButtonInner);
			world.DestroyEntity(playButton);
			world.DestroyEntity(background);
			world.DestroyEntity(middleContainer);
		}
	};
};

class UISystem : public System {
	public:
	Phase GetPhase() const override { return Phase::Input; }

	void OnStart() override {
		MessageCenter::Subscribe<AnimationEndEvent, &UISystem::OnAnimationEnd>(this);
		MessageCenter::Subscribe<LevelLoadedEvent, &UISystem::OnLevelLoaded>(this);
		MessageCenter::Subscribe<ButtonHoverEvent, &UISystem::OnButtonHover>(this);
		MessageCenter::Subscribe<ButtonClickEvent, &UISystem::OnButtonPress>(this);
		MessageCenter::Subscribe<ObjectiveAchievedEvent, &UISystem::OnObjectiveComplete>(this);

		createSplashScreen();

		std::thread([this]() {
			fontManager.loadFont(std::filesystem::path(PROJECT_ASSETS_PATH) / "fonts/Chewy-Regular.ttf", "chewy-regular");
			textureManager.loadTexture(std::filesystem::path(PROJECT_ASSETS_PATH) / "interface/tick.png", "Tick");

			MessageCenter::Publish(RequestLevelChangeEvent({
				std::filesystem::path(PROJECT_ASSETS_PATH) / "level_1_h1.glb",
				std::filesystem::path(PROJECT_ASSETS_PATH) / "level_1_h2.glb"
				}));

			createMainMenu();
			initialised = true;
		}).detach();
	}

	void OnUpdate(float dt) override {
		if (currentScreenType == ScreenType::LEVEL) {
			LevelScreen* screen = static_cast<LevelScreen*>(currentScreen);

			if (!screen->paused) {
				// no idea why a red rectangle appears when you click the resume button *and ONLY when you click it* so this is the best fix i could think of
				registry.get<BackgroundComponent>(screen->playButton).transparency = 1.0f;
				registry.get<BackgroundComponent>(screen->playButtonInner).transparency = 1.0f;

				registry.get<BackgroundComponent>(screen->respawnButton).transparency = 1.0f;
				registry.get<BackgroundComponent>(screen->respawnButtonInner).transparency = 1.0f;

				registry.get<BackgroundComponent>(screen->menuButton).transparency = 1.0f;
				registry.get<BackgroundComponent>(screen->menuButtonInner).transparency = 1.0f;

				registry.get<BackgroundComponent>(screen->quitGameButton).transparency = 1.0f;
				registry.get<BackgroundComponent>(screen->quitGameButtonInner).transparency = 1.0f;
			}

			if (inputManager.isKeyJustDown(GLFW_KEY_ESCAPE)) {
				TogglePause();
			}
		}

		if (nextReady && initialised) {
			nextReady = false;
			if (currentScreenType == ScreenType::SPLASH) {
				SplashScreen* screen = static_cast<SplashScreen*>(currentScreen);
				InterfaceAnimationComponent animComp;
				animComp.copy(screen->engineLogo);
				animComp.targetTextureColour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
				animComp.time = 2.0f;
				animComp.delay = 1.0f;
				animComp.reverse = true;
				animComp.interpolation = InterfaceInterpolationType::EASE_IN;
				{
					std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
					registry.emplace<InterfaceAnimationComponent>(screen->engineLogo, animComp);
					registry.erase<InterfaceAnimationComponent>(screen->spinner);
				}

				InterfaceAnimationComponent spinnerComp;
				spinnerComp.copy(screen->spinner);
				spinnerComp.targetTextureColour = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
				spinnerComp.targetRotation += 180.0f;
				spinnerComp.time = 0.5f;
				spinnerComp.interpolation = InterfaceInterpolationType::EASE_OUT;
				{
					std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
					registry.emplace<InterfaceAnimationComponent>(screen->spinner, spinnerComp);
				}
			} else if (currentScreenType == ScreenType::LOADING) {
				LoadingScreen* screen = static_cast<LoadingScreen*>(currentScreen);
				InterfaceAnimationComponent animComp;
				animComp.copy(screen->background);
				animComp.targetBackgroundTransparency = 1.0f;
				animComp.time = 1.0f;
				animComp.interpolation = InterfaceInterpolationType::EASE_OUT;
				{
					std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
					registry.emplace<InterfaceAnimationComponent>(screen->background, animComp);
					registry.erase<InterfaceAnimationComponent>(screen->spinner);
				}

				InterfaceAnimationComponent spinnerComp;
				spinnerComp.copy(screen->spinner);
				spinnerComp.targetTextureColour = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
				spinnerComp.targetRotation += 180.0f;
				spinnerComp.time = 0.5f;
				spinnerComp.interpolation = InterfaceInterpolationType::EASE_OUT;
				{
					std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
					registry.emplace<InterfaceAnimationComponent>(screen->spinner, spinnerComp);
				}

				if (nextScreenType == ScreenType::LEVEL) {
					inputManager.disableCursor();

					MessageCenter::Publish(ResetLevelEvent(static_cast<LevelScreen*>(nextScreen)->timer));
				}
			}
		}
	}

	void OnAnimationEnd(const AnimationEndEvent& e) {
		switch (currentScreenType) {
		case (ScreenType::SPLASH):
			{
				SplashScreen* screen = static_cast<SplashScreen*>(currentScreen);
				if (e.entity == screen->engineLogo) {
					InterfaceAnimationComponent animComp;
					animComp.copy(screen->gameLogo);
					animComp.targetTextureColour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
					animComp.time = 2.0f;
					animComp.delay = 1.0f;
					animComp.reverse = true;
					animComp.interpolation = InterfaceInterpolationType::EASE_IN;
					{
						std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
						registry.emplace<InterfaceAnimationComponent>(screen->gameLogo, animComp);
					}
				} else if (e.entity == screen->gameLogo) {
					InterfaceAnimationComponent animComp;
					animComp.copy(screen->background);
					animComp.targetBackgroundTransparency = 1.0f;
					animComp.time = 1.0f;
					animComp.interpolation = InterfaceInterpolationType::EASE_OUT;
					{
						std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
						registry.emplace<InterfaceAnimationComponent>(screen->background, animComp);
					}
				} else if (e.entity == screen->background) {
					delete currentScreen;
					currentScreen = std::move(nextScreen);
					currentScreenType = ScreenType::MAIN_MENU;
					nextScreen = new ScreenBase();
				}
			}
			break;
		case (ScreenType::MAIN_MENU):
			{
				if (nextScreenType == ScreenType::LOADING) {
					delete currentScreen;
					currentScreen = std::move(nextScreen);
					currentScreenType = ScreenType::LOADING;
					nextScreen = new ScreenBase();
					LoadingScreen* screen = static_cast<LoadingScreen*>(currentScreen);

					InterfaceAnimationComponent animComp;
					animComp.copy(screen->spinner);
					animComp.targetRotation += 360.0f;
					animComp.loop = true;
					animComp.time = 1.0f;
					{
						std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
						registry.emplace<InterfaceAnimationComponent>(screen->spinner, animComp);
					}

					if (levelToLoad == 1) {
						MessageCenter::Publish(RequestLevelChangeEvent({
							std::filesystem::path(PROJECT_ASSETS_PATH) / "level_1_h1.glb",
							std::filesystem::path(PROJECT_ASSETS_PATH) / "level_1_h2.glb",
							std::filesystem::path(PROJECT_ASSETS_PATH) / "kiki_player.glb",
							//std::filesystem::path(PROJECT_ASSETS_PATH) / "tele.glb",
						}));
					} else {
						MessageCenter::Publish(RequestLevelChangeEvent({
							std::filesystem::path(PROJECT_ASSETS_PATH) / "level2.glb",
							std::filesystem::path(PROJECT_ASSETS_PATH) / "kiki_player.glb"
						}));
					}

					createLevelScreen();
				}
			}
			break;
		case (ScreenType::LOADING):
			{
				LoadingScreen* screen = static_cast<LoadingScreen*>(currentScreen);
				if (e.entity == screen->background && registry.get<BackgroundComponent>(e.entity).transparency == 1.0f) {
					delete currentScreen;
					currentScreen = std::move(nextScreen);
					currentScreenType = nextScreenType;
					nextScreen = new ScreenBase();

					if (currentScreenType == ScreenType::LEVEL) {
						addObjectives();
					}
				}
			}
			break;
		case (ScreenType::LEVEL):
			{
				if (nextScreenType == ScreenType::LOADING) {
					LoadingScreen* screen = static_cast<LoadingScreen*>(nextScreen);

					if (e.entity == screen->background && registry.get<BackgroundComponent>(e.entity).transparency == 0.0f) {
						delete currentScreen;
						currentScreen = std::move(nextScreen);
						currentScreenType = ScreenType::LOADING;
						nextScreen = new ScreenBase();
						LoadingScreen* screen = static_cast<LoadingScreen*>(currentScreen);

						if (registry.all_of<InterfaceAnimationComponent>(screen->spinner)) {
							{
								std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
								registry.erase<InterfaceAnimationComponent>(screen->spinner);
							}
						}

						InterfaceAnimationComponent animComp;
						animComp.copy(screen->spinner);
						animComp.targetRotation += 360.0f;
						animComp.loop = true;
						animComp.time = 1.0f;
						{
							std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
							registry.emplace<InterfaceAnimationComponent>(screen->spinner, animComp);
						}

						MessageCenter::Publish(RequestLevelChangeEvent({
							std::filesystem::path(PROJECT_ASSETS_PATH) / "level2.glb",
							//std::filesystem::path(PROJECT_ASSETS_PATH) / "level_l_h2.glb"
							//std::filesystem::path(PROJECT_ASSETS_PATH) / "tele.glb",
						}));

						createMainMenu();
					}
				}
			}
			break;
		}
	}

	void OnLevelLoaded(const LevelLoadedEvent& e) {
		nextReady = true;
	}

	void OnButtonHover(const ButtonHoverEvent& e) {
		if (currentScreenType != ScreenType::SPLASH && currentScreenType != ScreenType::LOADING)
			Kiki::AudioSystem::PlayOneShot("interface/button.wav");
	}

	void OnButtonPress(const ButtonClickEvent& e) {
		if (e.clickType == ClickType::BUTTON_RELEASE) {
			if (currentScreenType == ScreenType::MAIN_MENU) {
				MainMenuScreen* screen = static_cast<MainMenuScreen*>(currentScreen);

				if (e.button == screen->playButton) {
					{
						std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
						registry.erase<ButtonComponent>(screen->playButton);
						registry.erase<ButtonComponent>(screen->play2Button);
					}
					levelToLoad = 1;
					createLoadingScreen();
				} else if (e.button == screen->play2Button) {
					{
						std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
						registry.erase<ButtonComponent>(screen->playButton);
						registry.erase<ButtonComponent>(screen->play2Button);
					}
					levelToLoad = 2;
					createLoadingScreen();
				} else if (e.button == screen->quitGameButton) {
					glfwSetWindowShouldClose(renderManager.getWindow(), true);
				}
			} else if (currentScreenType == ScreenType::LEVEL) {
				LevelScreen* screen = static_cast<LevelScreen*>(currentScreen);

				if (e.button == screen->playButton) {
					TogglePause();
				} else if (e.button == screen->respawnButton) {
					TogglePause();
					MessageCenter::Publish(RespawnCharacterEvent());
				} else if (e.button == screen->menuButton) {
					{
						std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
						auto view = world.Query<CharacterComponent>();

						for (auto [e, comp] : view.each()) {
							registry.erase<CharacterComponent>(e);
						}
					}

					createLoadingScreen();
				} else if (e.button == screen->quitGameButton) {
					glfwSetWindowShouldClose(renderManager.getWindow(), true);
				}
			}
		}
	}

	void OnObjectiveComplete(ObjectiveAchievedEvent e) {
		if (currentScreenType == ScreenType::LEVEL) {
			int i = e.objective;
			LevelScreen* screen = static_cast<LevelScreen*>(currentScreen);

			if (!screen->objectives[i].complete) {
				InterfaceAnimationComponent strikethroughComp;
				strikethroughComp.copy(screen->objectives[i].strikethrough);
				strikethroughComp.targetSize = ScaleVec2D(1.0f, -20.0f, 0.0f, 4.0f);
				strikethroughComp.time = 1.0f;
				strikethroughComp.interpolation = InterfaceInterpolationType::EASE_IN_OUT;
				{
					std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
					registry.emplace<InterfaceAnimationComponent>(screen->objectives[i].strikethrough, strikethroughComp);
				}

				InterfaceAnimationComponent tickComp;
				tickComp.copy(screen->objectives[i].tick);
				tickComp.targetPosition = ScaleVec2D(0.0f, -45.0f, 0.0f, tickComp.targetPosition.y);
				tickComp.targetSize = ScaleVec2D(0.0f, 40.0f, 0.0f, 40.0f);
				tickComp.targetRotation += 720.0f;
				tickComp.targetTextureColour = glm::vec4(52.0f / 255.0f, 181.0f / 255.0f, 88.0f / 255.0f, 1.0f);
				tickComp.time = 1.0f;
				tickComp.interpolation = InterfaceInterpolationType::EASE_IN_OUT;
				{
					std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
					registry.emplace<InterfaceAnimationComponent>(screen->objectives[i].tick, tickComp);
				}

				InterfaceAnimationComponent textComp;
				textComp.copy(screen->objectives[i].time);
				textComp.targetTextColour = glm::vec3(0.8f, 0.8f, 0.8f);
				textComp.targetTextTransparency = 0.5f;
				textComp.time = 1.0f;
				tickComp.interpolation = InterfaceInterpolationType::EASE_IN_OUT;
				{
					std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
					registry.emplace<InterfaceAnimationComponent>(screen->objectives[i].time, textComp);
				}
				textComp.copy(screen->objectives[i].powerup);
				textComp.targetTextColour = glm::vec3(0.8f, 0.8f, 0.8f);
				textComp.targetTextTransparency = 0.5f;
				textComp.time = 1.0f;
				tickComp.interpolation = InterfaceInterpolationType::EASE_IN_OUT;
				{
					std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
					registry.emplace<InterfaceAnimationComponent>(screen->objectives[i].powerup, textComp);
				}

				Kiki::AudioSystem::PlayOneShot("interface/success.wav");
			}
		}
	}

	void TogglePause() {
		if (currentScreenType == ScreenType::LEVEL) {
			LevelScreen* screen = static_cast<LevelScreen*>(currentScreen);
			
			if (screen->paused) {
				inputManager.disableCursor();

				registry.get<BackgroundComponent>(screen->background).transparency = 1.0f;
				registry.get<BackgroundComponent>(screen->playButton).transparency = 1.0f;
				registry.get<BackgroundComponent>(screen->playButtonInner).transparency = 1.0f;
				registry.get<TextComponent>(screen->playButtonInner).transparency = 1.0f;
				registry.get<BackgroundComponent>(screen->respawnButton).transparency = 1.0f;
				registry.get<BackgroundComponent>(screen->respawnButtonInner).transparency = 1.0f;
				registry.get<TextComponent>(screen->respawnButtonInner).transparency = 1.0f;
				registry.get<BackgroundComponent>(screen->menuButton).transparency = 1.0f;
				registry.get<BackgroundComponent>(screen->menuButtonInner).transparency = 1.0f;
				registry.get<TextComponent>(screen->menuButtonInner).transparency = 1.0f;
				registry.get<BackgroundComponent>(screen->quitGameButton).transparency = 1.0f;
				registry.get<BackgroundComponent>(screen->quitGameButtonInner).transparency = 1.0f;
				registry.get<TextComponent>(screen->quitGameButtonInner).transparency = 1.0f;

				{
					std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
					registry.erase<ButtonComponent>(screen->playButton);
					registry.erase<ButtonComponent>(screen->respawnButton);
					registry.erase<ButtonComponent>(screen->menuButton);
					registry.erase<ButtonComponent>(screen->quitGameButton);
				}

				screen->paused = false;
			} else {
				inputManager.enableCursor();

				registry.get<BackgroundComponent>(screen->background).transparency = 0.3f;
				registry.get<BackgroundComponent>(screen->playButton).transparency = 0.8f;
				registry.get<BackgroundComponent>(screen->playButtonInner).transparency = 0.6f;
				registry.get<TextComponent>(screen->playButtonInner).transparency = 0.0f;
				registry.get<BackgroundComponent>(screen->respawnButton).transparency = 0.8f;
				registry.get<BackgroundComponent>(screen->respawnButtonInner).transparency = 0.6f;
				registry.get<TextComponent>(screen->respawnButtonInner).transparency = 0.0f;
				registry.get<BackgroundComponent>(screen->menuButton).transparency = 0.8f;
				registry.get<BackgroundComponent>(screen->menuButtonInner).transparency = 0.6f;
				registry.get<TextComponent>(screen->menuButtonInner).transparency = 0.0f;
				registry.get<BackgroundComponent>(screen->quitGameButton).transparency = 0.8f;
				registry.get<BackgroundComponent>(screen->quitGameButtonInner).transparency = 0.6f;
				registry.get<TextComponent>(screen->quitGameButtonInner).transparency = 0.0f;
				
				{
					std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
					registry.emplace<ButtonComponent>(screen->playButton, glm::vec4(20.0f / 255.0f, 70.0f / 255.0f, 120.0f / 255.0f, 0.2f), glm::vec4(29.0f / 255.0f, 100.0f / 255.0f, 171.0f / 255.0f, 0.2f), glm::vec4(12.0f / 255.0f, 40.0f / 255.0f, 69.0f / 255.0f, 0.2f));
					registry.emplace<ButtonComponent>(screen->respawnButton, glm::vec4(20.0f / 255.0f, 70.0f / 255.0f, 120.0f / 255.0f, 0.2f), glm::vec4(29.0f / 255.0f, 100.0f / 255.0f, 171.0f / 255.0f, 0.2f), glm::vec4(12.0f / 255.0f, 40.0f / 255.0f, 69.0f / 255.0f, 0.2f));
					registry.emplace<ButtonComponent>(screen->menuButton, glm::vec4(20.0f / 255.0f, 70.0f / 255.0f, 120.0f / 255.0f, 0.2f), glm::vec4(29.0f / 255.0f, 100.0f / 255.0f, 171.0f / 255.0f, 0.2f), glm::vec4(12.0f / 255.0f, 40.0f / 255.0f, 69.0f / 255.0f, 0.2f));
					registry.emplace<ButtonComponent>(screen->quitGameButton, glm::vec4(170.0f / 255.0f, 27.0f / 255.0f, 27.0f / 255.0f, 0.2f), glm::vec4(222.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 0.2f), glm::vec4(120.0f / 255.0f, 19.0f / 255.0f, 19.0f / 255.0f, 0.2f));
				}

				screen->paused = true;
			}
		}
	}

	private:
	World& world = World::Get();
	entt::registry& registry = world.Registry();
	TextureManager& textureManager = Kiki::TextureManager::get();
	FontManager& fontManager = Kiki::FontManager::get();
	SceneManager& sceneManager = Kiki::SceneManager::get();
	RenderManager& renderManager = Kiki::RenderManager::get();
	InputManager& inputManager = Kiki::InputManager::get();

	ScreenBase* currentScreen;
	ScreenBase* nextScreen;
	ScreenType currentScreenType;
	ScreenType nextScreenType;
	bool initialised = false;
	bool nextReady = false;
	int levelToLoad = 1;

	void createSplashScreen() {
		currentScreen = new SplashScreen();
		currentScreenType = ScreenType::SPLASH;
		SplashScreen* screen = static_cast<SplashScreen*>(currentScreen);

		{
			std::lock_guard<std::mutex> lock(sceneManager.registryMutex);

			screen->background = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->background, ScaleVec2D(0.0f, 0.0f, 0.0f, 0.0f), ScaleVec2D(1.0f, 0.0f, 1.0f, 0.0f), entt::null, (unsigned int) 60);
			registry.emplace<BackgroundComponent>(screen->background, glm::vec3(0.0f, 0.0f, 0.0f), 0.0f);

			textureManager.loadTexture(std::filesystem::path(PROJECT_ASSETS_PATH) / "interface/kiki_logo_transparent.png", "EngineLogoTransparent");
			screen->engineLogo = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->engineLogo, ScaleVec2D(0.25f, 0.0f, 0.0f, 0.0f), ScaleVec2D(0.5f, 0.0f, 1.0f, 0.0f), entt::null, (unsigned int) 61);
			registry.emplace<InterfaceTextureComponent>(screen->engineLogo, "EngineLogoTransparent", glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));
			registry.emplace<AspectRatioComponent>(screen->engineLogo, 1920.0f / 980.0f);

			textureManager.loadTexture(std::filesystem::path(PROJECT_ASSETS_PATH) / "interface/game_logo_transparent.png", "GameLogoTransparent");
			screen->gameLogo = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->gameLogo, ScaleVec2D(0.25f, 0.0f, 0.0f, 0.0f), ScaleVec2D(0.5f, 0.0f, 1.0f, 0.0f), entt::null, (unsigned int) 61);
			registry.emplace<InterfaceTextureComponent>(screen->gameLogo, "GameLogoTransparent", glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));
			registry.emplace<AspectRatioComponent>(screen->gameLogo, 1920.0f / 980.0f);

			textureManager.loadTexture(std::filesystem::path(PROJECT_ASSETS_PATH) / "interface/spinner.png", "Spinner");
			screen->spinner = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->spinner, ScaleVec2D(0.5f, -25.0f, 1.0f, -200.0f), ScaleVec2D(0.0f, 50.0f, 0.0f, 50.0f), entt::null, (unsigned int) 61);
			registry.emplace<InterfaceTextureComponent>(screen->spinner, "Spinner", glm::vec4(1.0f, 1.0f, 1.0f, 0.6f));
			InterfaceAnimationComponent animComp;
			animComp.copy(screen->spinner);
			animComp.targetRotation = 360.0f;
			animComp.loop = true;
			animComp.time = 1.0f;
			registry.emplace<InterfaceAnimationComponent>(screen->spinner, animComp);
		}
	}

	void createLoadingScreen() {
		nextScreen = new LoadingScreen();
		nextScreenType = ScreenType::LOADING;
		LoadingScreen* screen = static_cast<LoadingScreen*>(nextScreen);

		inputManager.enableCursor();

		{
			std::lock_guard<std::mutex> lock(sceneManager.registryMutex);

			screen->background = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->background, ScaleVec2D(0.0f, 0.0f, 0.0f, 0.0f), ScaleVec2D(1.0f, 0.0f, 1.0f, 0.0f), entt::null, (unsigned int)60);
			registry.emplace<BackgroundComponent>(screen->background, glm::vec3(0.0f, 0.0f, 0.0f), 1.0f);
			InterfaceAnimationComponent backgroundAnim;
			backgroundAnim.copy(screen->background);
			backgroundAnim.targetBackgroundTransparency = 0.0f;
			backgroundAnim.time = 1.0f;
			backgroundAnim.interpolation = InterfaceInterpolationType::EASE_IN;
			registry.emplace<InterfaceAnimationComponent>(screen->background, backgroundAnim);

			textureManager.loadTexture(std::filesystem::path(PROJECT_ASSETS_PATH) / "interface/spinner.png", "Spinner");
			screen->spinner = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->spinner, ScaleVec2D(0.5f, -25.0f, 1.0f, -200.0f), ScaleVec2D(0.0f, 50.0f, 0.0f, 50.0f), entt::null, (unsigned int)61);
			registry.emplace<InterfaceTextureComponent>(screen->spinner, "Spinner", glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));
			InterfaceAnimationComponent spinnerComp;
			spinnerComp.copy(screen->spinner);
			spinnerComp.targetTextureColour = glm::vec4(1.0f, 1.0f, 1.0f, 0.6f);
			spinnerComp.targetRotation += 360.0f;
			spinnerComp.time = 1.0f;
			registry.emplace<InterfaceAnimationComponent>(screen->spinner, spinnerComp);
		}
	}

	void createMainMenu() {
		nextScreen = new MainMenuScreen();
		nextScreenType = ScreenType::MAIN_MENU;
		MainMenuScreen* screen = static_cast<MainMenuScreen*>(nextScreen);

		auto cameras = world.Query<CameraComponent>();

		for (auto [e, cam] : cameras.each()) {
			cam.isMain = false;
		}

		{
			std::lock_guard<std::mutex> lock(sceneManager.registryMutex);

			screen->camera = new Camera;
			auto& camTransform = registry.get<TransformComponent>(screen->camera->camera);
			camTransform.position = glm::vec3(-37.494f, 35.741f, -26.41f);
			camTransform.rotation = glm::quat(glm::vec3(glm::radians(145.200f), glm::radians(-57.050f), glm::radians(-180.0f)));
			registry.get<CameraComponent>(screen->camera->camera).isMain = true;

			screen->middleContainer = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->middleContainer, ScaleVec2D(0.3f, 0.0f, 0.0f, 0.0f), ScaleVec2D(0.4f, 0.0f, 1.0f, 0.0f), entt::null, (unsigned int) 40);

			screen->gameLogo = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->gameLogo, ScaleVec2D(-0.1f, 0.0f, 0.1f, 0.0f), ScaleVec2D(1.2f, 0.0f, 0.25f, 0.0f), screen->middleContainer, (unsigned int) 40);
			registry.emplace<InterfaceTextureComponent>(screen->gameLogo, "GameLogoTransparent", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			registry.emplace<AspectRatioComponent>(screen->gameLogo, 1920.0f / 980.0f);
			InterfaceAnimationComponent logoBounce;
			logoBounce.copy(screen->gameLogo);
			logoBounce.targetPosition = ScaleVec2D(-0.1f, -25.0f, 0.1f, -25.0f);
			logoBounce.targetSize = ScaleVec2D(1.2f, 50.0f, 0.25f, 50.0f);
			logoBounce.time = 5.0f;
			logoBounce.reverse = true;
			logoBounce.loop = true;
			logoBounce.interpolation = InterfaceInterpolationType::EASE_IN_OUT;
			registry.emplace<InterfaceAnimationComponent>(screen->gameLogo, logoBounce);

			screen->background = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->background, ScaleVec2D(0.0f, 0.0f, 0.45f, 0.0f), ScaleVec2D(1.0f, 0.0f, 0.3f, 0.0f), screen->middleContainer, (unsigned int) 40);
			registry.emplace<BackgroundComponent>(screen->background, glm::vec3(0.1f, 0.1f, 0.1f), 0.3f, 40.0f);
			registry.emplace<AspectRatioComponent>(screen->background, 3.0f / 2.7f);

			screen->playButton = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->playButton, ScaleVec2D(0.0f, 12.0f, 0.0f, 12.0f), ScaleVec2D(1.0f, -24.0f, (1.0f / 3.0f), -16.0f), screen->background, (unsigned int) 41);
			registry.emplace<BackgroundComponent>(screen->playButton, glm::vec3(20.0f / 255.0f, 70.0f / 255.0f, 120.0f / 255.0f), 0.8f, 28.0f);
			registry.emplace<ButtonComponent>(screen->playButton, glm::vec4(20.0f / 255.0f, 70.0f / 255.0f, 120.0f / 255.0f, 0.2f), glm::vec4(29.0f / 255.0f, 100.0f / 255.0f, 171.0f / 255.0f, 0.2f), glm::vec4(12.0f / 255.0f, 40.0f / 255.0f, 69.0f / 255.0f, 0.2f));

			screen->playButtonInner = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->playButtonInner, ScaleVec2D(0.0f, 12.0f, 0.0f, 12.0f), ScaleVec2D(1.0f, -24.0f, 1.0f, -24.0f), screen->playButton, (unsigned int) 42);
			registry.emplace<BackgroundComponent>(screen->playButtonInner, glm::vec3(20.0f / 255.0f, 70.0f / 255.0f, 120.0f / 255.0f), 0.6f, 16.0f);
			registry.emplace<TextComponent>(screen->playButtonInner, "chewy-regular", U"Play Level 1", 0.6f, glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, Kiki::HorizontalAlignment::CENTRE, Kiki::VerticalAlignment::CENTRE, true);

			screen->play2Button = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->play2Button, ScaleVec2D(0.0f, 12.0f, (1.0f / 3.0f), 8.0f), ScaleVec2D(1.0f, -24.0f, (1.0f / 3.0f), -16.0f), screen->background, (unsigned int) 41);
			registry.emplace<BackgroundComponent>(screen->play2Button, glm::vec3(20.0f / 255.0f, 70.0f / 255.0f, 120.0f / 255.0f), 0.8f, 28.0f);
			registry.emplace<ButtonComponent>(screen->play2Button, glm::vec4(20.0f / 255.0f, 70.0f / 255.0f, 120.0f / 255.0f, 0.2f), glm::vec4(29.0f / 255.0f, 100.0f / 255.0f, 171.0f / 255.0f, 0.2f), glm::vec4(12.0f / 255.0f, 40.0f / 255.0f, 69.0f / 255.0f, 0.2f));

			screen->play2ButtonInner = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->play2ButtonInner, ScaleVec2D(0.0f, 12.0f, 0.0f, 12.0f), ScaleVec2D(1.0f, -24.0f, 1.0f, -24.0f), screen->play2Button, (unsigned int) 42);
			registry.emplace<BackgroundComponent>(screen->play2ButtonInner, glm::vec3(20.0f / 255.0f, 70.0f / 255.0f, 120.0f / 255.0f), 0.6f, 16.0f);
			registry.emplace<TextComponent>(screen->play2ButtonInner, "chewy-regular", U"Play Level 2", 0.6f, glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, Kiki::HorizontalAlignment::CENTRE, Kiki::VerticalAlignment::CENTRE, true);

			screen->quitGameButton = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->quitGameButton, ScaleVec2D(0.0f, 12.0f, (2.0f / 3.0f), 4.0f), ScaleVec2D(1.0f, -24.0f, (1.0f / 3.0f), -16.0f), screen->background, (unsigned int) 41);
			registry.emplace<BackgroundComponent>(screen->quitGameButton, glm::vec3(170.0f / 255.0f, 27.0f / 255.0f, 27.0f / 255.0f), 0.8f, 28.0f);
			registry.emplace<ButtonComponent>(screen->quitGameButton, glm::vec4(170.0f / 255.0f, 27.0f / 255.0f, 27.0f / 255.0f, 0.2f), glm::vec4(222.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 0.2f), glm::vec4(120.0f / 255.0f, 19.0f / 255.0f, 19.0f / 255.0f, 0.2f));

			screen->quitGameButtonInner = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->quitGameButtonInner, ScaleVec2D(0.0f, 12.0f, 0.0f, 12.0f), ScaleVec2D(1.0f, -24.0f, 1.0f, -24.0f), screen->quitGameButton, (unsigned int)42);
			registry.emplace<BackgroundComponent>(screen->quitGameButtonInner, glm::vec3(170.0f / 255.0f, 27.0f / 255.0f, 27.0f / 255.0f), 0.4f, 16.0f);
			registry.emplace<TextComponent>(screen->quitGameButtonInner, "chewy-regular", U"Quit", 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, Kiki::HorizontalAlignment::CENTRE, Kiki::VerticalAlignment::CENTRE, true);
		}
	}

	void createLevelScreen() {
		nextScreen = new LevelScreen();
		nextScreenType = ScreenType::LEVEL;
		LevelScreen* screen = static_cast<LevelScreen*>(nextScreen);

		{
			std::lock_guard<std::mutex> lock(sceneManager.registryMutex);

			screen->timerBackground = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->timerBackground, ScaleVec2D(0.0f, 0.0f, 0.0f, 10.0f), ScaleVec2D(1.0f, 0.0f, 0.1f, 0.0f), entt::null, (unsigned int) 40);
			registry.emplace<BackgroundComponent>(screen->timerBackground, glm::vec3(0.1f, 0.1f, 0.1f), 0.3f, 40.0f);
			registry.emplace<AspectRatioComponent>(screen->timerBackground, 3.0f / 1.0f);

			screen->timer = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->timer, ScaleVec2D(0.0f, 0.0f, 0.0f, 0.0f), ScaleVec2D(1.0f, 0.0f, 1.0f, 0.0f), screen->timerBackground, (unsigned int) 41);
			registry.emplace<TextComponent>(screen->timer, "chewy-regular", U"00:00.00", 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, Kiki::HorizontalAlignment::CENTRE, Kiki::VerticalAlignment::CENTRE, true);

			screen->objectivesContainer = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->objectivesContainer, ScaleVec2D(1.0f, -300.0f, 0.5f, -52.0f), ScaleVec2D(0.0f, 300.0f, 0.0f, 104.0f), entt::null, (unsigned int)40);

			screen->objectivesBackground = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->objectivesBackground, ScaleVec2D(0.0f, 0.0f, 0.0f, 0.0f), ScaleVec2D(1.0f, 20.0f, 1.0f, 00.0f), screen->objectivesContainer, (unsigned int)40);
			registry.emplace<BackgroundComponent>(screen->objectivesBackground, glm::vec3(0.1f, 0.1f, 0.1f), 0.3f, 20.0f);

			screen->objectivesTitle = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->objectivesTitle, ScaleVec2D(0.0f, 0.0f, 0.0f, 0.0f), ScaleVec2D(1.0f, 0.0f, 0.0f, 50.0f), screen->objectivesContainer, (unsigned int)41);
			registry.emplace<TextComponent>(screen->objectivesTitle, "chewy-regular", U"Objectives", 0.7f, glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, Kiki::HorizontalAlignment::CENTRE, Kiki::VerticalAlignment::CENTRE, true);

			screen->objectivesTitleDivider = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->objectivesTitleDivider, ScaleVec2D(0.0f, 10.0f, 0.0f, 50.0f), ScaleVec2D(1.0f, -20.0f, 0.0f, 2.0f), screen->objectivesContainer, (unsigned int)41);
			registry.emplace<BackgroundComponent>(screen->objectivesTitleDivider, glm::vec3(0.9f, 0.9f, 0.9f), 0.3f, 1.0f);

			screen->objectivesTimeLimit = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->objectivesTimeLimit, ScaleVec2D(0.0f, 0.0f, 0.0f, 52.0f), ScaleVec2D(0.0f, 100.0f, 0.0f, 40.0f), screen->objectivesContainer, (unsigned int)41);
			registry.emplace<TextComponent>(screen->objectivesTimeLimit, "chewy-regular", U"Time Limit", 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, Kiki::HorizontalAlignment::CENTRE, Kiki::VerticalAlignment::CENTRE, true);

			screen->objectivesPowerup = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->objectivesPowerup, ScaleVec2D(0.0f, 100.0f, 0.0f, 52.0f), ScaleVec2D(0.0f, 200.0f, 0.0f, 40.0f), screen->objectivesContainer, (unsigned int)41);
			registry.emplace<TextComponent>(screen->objectivesPowerup, "chewy-regular", U"Powerup", 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, Kiki::HorizontalAlignment::CENTRE, Kiki::VerticalAlignment::CENTRE, true);

			screen->objectivesSubtitleDivider = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->objectivesSubtitleDivider, ScaleVec2D(0.0f, 10.0f, 0.0f, 92.0f), ScaleVec2D(1.0f, -20.0f, 0.0f, 2.0f), screen->objectivesContainer, (unsigned int)41);
			registry.emplace<BackgroundComponent>(screen->objectivesSubtitleDivider, glm::vec3(0.9f, 0.9f, 0.9f), 0.3f, 1.0f);

			// Pause menu
			screen->middleContainer = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->middleContainer, ScaleVec2D(0.3f, 0.0f, 0.3f, 0.0f), ScaleVec2D(0.4f, 0.0f, 0.4f, 0.0f), entt::null, (unsigned int)40);

			screen->background = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->background, ScaleVec2D(0.0f, 0.0f, 0.0f, 0.0f), ScaleVec2D(1.0f, 0.0f, 1.0f, 0.0f), screen->middleContainer, (unsigned int)40);
			registry.emplace<BackgroundComponent>(screen->background, glm::vec3(0.1f, 0.1f, 0.1f), 1.0f, 40.0f);
			registry.emplace<AspectRatioComponent>(screen->background, 3.0f / 3.6f);

			screen->playButton = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->playButton, ScaleVec2D(0.0f, 12.0f, 0.0f, 12.0f), ScaleVec2D(1.0f, -24.0f, 0.25f, -15.0f), screen->background, (unsigned int)41);
			registry.emplace<BackgroundComponent>(screen->playButton, glm::vec3(20.0f / 255.0f, 70.0f / 255.0f, 120.0f / 255.0f), 1.0f, 28.0f);

			screen->playButtonInner = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->playButtonInner, ScaleVec2D(0.0f, 12.0f, 0.0f, 12.0f), ScaleVec2D(1.0f, -24.0f, 1.0f, -24.0f), screen->playButton, (unsigned int)42);
			registry.emplace<BackgroundComponent>(screen->playButtonInner, glm::vec3(20.0f / 255.0f, 70.0f / 255.0f, 120.0f / 255.0f), 1.0f, 16.0f);
			registry.emplace<TextComponent>(screen->playButtonInner, "chewy-regular", U"Resume", 0.6f, glm::vec3(1.0f, 1.0f, 1.0f), 1.0f, Kiki::HorizontalAlignment::CENTRE, Kiki::VerticalAlignment::CENTRE, true);

			screen->respawnButton = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->respawnButton, ScaleVec2D(0.0f, 12.0f, 0.25f, 9.0f), ScaleVec2D(1.0f, -24.0f, 0.25f, -15.0f), screen->background, (unsigned int)41);
			registry.emplace<BackgroundComponent>(screen->respawnButton, glm::vec3(20.0f / 255.0f, 70.0f / 255.0f, 120.0f / 255.0f), 1.0f, 28.0f);

			screen->respawnButtonInner = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->respawnButtonInner, ScaleVec2D(0.0f, 12.0f, 0.0f, 12.0f), ScaleVec2D(1.0f, -24.0f, 1.0f, -24.0f), screen->respawnButton, (unsigned int)42);
			registry.emplace<BackgroundComponent>(screen->respawnButtonInner, glm::vec3(20.0f / 255.0f, 70.0f / 255.0f, 120.0f / 255.0f), 1.0f, 16.0f);
			registry.emplace<TextComponent>(screen->respawnButtonInner, "chewy-regular", U"Respawn", 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), 1.0f, Kiki::HorizontalAlignment::CENTRE, Kiki::VerticalAlignment::CENTRE, true);

			screen->menuButton = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->menuButton, ScaleVec2D(0.0f, 12.0f, 0.5f, 6.0f), ScaleVec2D(1.0f, -24.0f, 0.25f, -15.0f), screen->background, (unsigned int)41);
			registry.emplace<BackgroundComponent>(screen->menuButton, glm::vec3(20.0f / 255.0f, 70.0f / 255.0f, 120.0f / 255.0f), 1.0f, 28.0f);
			
			screen->menuButtonInner = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->menuButtonInner, ScaleVec2D(0.0f, 12.0f, 0.0f, 12.0f), ScaleVec2D(1.0f, -24.0f, 1.0f, -24.0f), screen->menuButton, (unsigned int)42);
			registry.emplace<BackgroundComponent>(screen->menuButtonInner, glm::vec3(20.0f / 255.0f, 70.0f / 255.0f, 120.0f / 255.0f), 1.0f, 16.0f);
			registry.emplace<TextComponent>(screen->menuButtonInner, "chewy-regular", U"Return to Menu", 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), 1.0f, Kiki::HorizontalAlignment::CENTRE, Kiki::VerticalAlignment::CENTRE, true);

			screen->quitGameButton = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->quitGameButton, ScaleVec2D(0.0f, 12.0f, 0.75f, 3.0f), ScaleVec2D(1.0f, -24.0f, 0.25f, -15.0f), screen->background, (unsigned int)41);
			registry.emplace<BackgroundComponent>(screen->quitGameButton, glm::vec3(170.0f / 255.0f, 27.0f / 255.0f, 27.0f / 255.0f), 1.0f, 28.0f);
			
			screen->quitGameButtonInner = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->quitGameButtonInner, ScaleVec2D(0.0f, 12.0f, 0.0f, 12.0f), ScaleVec2D(1.0f, -24.0f, 1.0f, -24.0f), screen->quitGameButton, (unsigned int)42);
			registry.emplace<BackgroundComponent>(screen->quitGameButtonInner, glm::vec3(170.0f / 255.0f, 27.0f / 255.0f, 27.0f / 255.0f), 1.0f, 16.0f);
			registry.emplace<TextComponent>(screen->quitGameButtonInner, "chewy-regular", U"Quit", 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), 1.0f, Kiki::HorizontalAlignment::CENTRE, Kiki::VerticalAlignment::CENTRE, true);
		}
	}

	std::u32string formatTime(float seconds) {
		int mins = static_cast<int>(seconds) / 60;
		int secs = static_cast<int>(seconds) % 60;
		char buf[12];
		snprintf(buf, sizeof(buf), "%d:%02d  ", mins, secs);

		std::u32string result;
		for (char c : std::string(buf)) result.push_back(static_cast<char32_t>(c));

		return result;
	}

	void addObjectives() {
		LevelScreen* screen = static_cast<LevelScreen*>(currentScreen);
		auto objects = world.Query<CharacterComponent>();

		std::vector<float> timeLimits;
		std::vector<std::u32string> labels;

		for (auto [entity, chara] : objects.each()) {
			timeLimits = chara.timeLimits;
			labels = chara.labels;
			break; // just take the first one we find, assuming there's only one player character
		}

		InterfaceAnimationComponent animComp;
		animComp.copy(screen->objectivesContainer);
		animComp.targetPosition = ScaleVec2D(1.0f, -300.0f, 0.5f, -0.5f * (94.0f + ((float)timeLimits.size() * 40.0f)));
		animComp.targetSize = ScaleVec2D(0.0f, 300.0f, 0.0f, 94.0f + ((float)timeLimits.size() * 40.0f));
		animComp.time = 1.5f;
		animComp.interpolation = InterfaceInterpolationType::EASE_IN_OUT; 
		{
			std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
			registry.emplace<InterfaceAnimationComponent>(screen->objectivesContainer, animComp);
		}

		for (int i = 0; i < timeLimits.size(); i++) {
			Objective obj;
			{
				std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
				obj.time = world.CreateEntity();
				registry.emplace<InterfaceComponent>(obj.time, ScaleVec2D(0.0f, 0.0f, 0.0f, 94.0f + (i * 40.0f)), ScaleVec2D(0.0f, 100.0f, 0.0f, 40.0f), screen->objectivesContainer, (unsigned int)41);
				registry.emplace<TextComponent>(obj.time, "chewy-regular", formatTime(timeLimits[i]), 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), 1.0f, Kiki::HorizontalAlignment::CENTRE, Kiki::VerticalAlignment::CENTRE, true);
			}
			InterfaceAnimationComponent timeComp;
			timeComp.copy(obj.time);
			timeComp.targetTextTransparency = 0.0f;
			timeComp.time = 3.0f;
			timeComp.interpolation = InterfaceInterpolationType::EASE_IN;
			{
				std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
				registry.emplace<InterfaceAnimationComponent>(obj.time, timeComp);
			}

			{
				std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
				obj.powerup = world.CreateEntity();
				registry.emplace<InterfaceComponent>(obj.powerup, ScaleVec2D(0.0f, 100.0f, 0.0f, 94.0f + (i * 40.0f)), ScaleVec2D(0.0f, 200.0f, 0.0f, 40.0f), screen->objectivesContainer, (unsigned int)41);
				registry.emplace<TextComponent>(obj.powerup, "chewy-regular", labels[i], 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), 1.0f, Kiki::HorizontalAlignment::CENTRE, Kiki::VerticalAlignment::CENTRE, true);
			}
			InterfaceAnimationComponent powerupComp;
			powerupComp.copy(obj.powerup);
			powerupComp.targetTextTransparency = 0.0f;
			powerupComp.time = 3.0f;
			powerupComp.interpolation = InterfaceInterpolationType::EASE_IN;
			{
				std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
				registry.emplace<InterfaceAnimationComponent>(obj.powerup, powerupComp);
			}

			{
				std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
				obj.strikethrough = world.CreateEntity();
				registry.emplace<InterfaceComponent>(obj.strikethrough, ScaleVec2D(0.0f, 10.0f, 0.0f, 112.0f + (i * 40.0f)), ScaleVec2D(0.0f, 0.0f, 0.0f, 4.0f), screen->objectivesContainer, (unsigned int)42);
				registry.emplace<BackgroundComponent>(obj.strikethrough, glm::vec3(52.0f / 255.0f, 181.0f / 255.0f, 88.0f / 255.0f), 0.3f, 2.0f);
			}

			{
				std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
				obj.tick = world.CreateEntity();
				registry.emplace<InterfaceComponent>(obj.tick, ScaleVec2D(0.0f, -60.0f, 0.0f, 94.0f + (i * 40.0f)), ScaleVec2D(0.0f, 0.0f, 0.0f, 0.0f), screen->objectivesContainer, (unsigned int) 42);
				registry.emplace<InterfaceTextureComponent>(obj.tick, "Tick", glm::vec4(52.0f / 255.0f, 181.0f / 255.0f, 88.0f / 255.0f, 0.0f));
			}
			screen->objectives.push_back(obj);
		}
		
	}
};

#endif