#include <kiki.h>


int main(int argc, char** argv) {
    Kiki::Engine engine;
    engine.Init();


    std::thread([]() {
        Kiki::SceneManager::get().loadScene(Kiki::GltfLoaderAssimp::loadScene(std::filesystem::path(PROJECT_ASSETS_PATH) / "sponza.glb"));
    }).detach();

    engine.Run();
}