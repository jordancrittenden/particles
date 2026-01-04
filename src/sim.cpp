#include <iostream>
#include "args.h"
#include "scene.h"
#include "free_space.h"
#include "tokamak.h"

// Main function
int main(int argc, char* argv[]) {
    SimulationParams params;
#if !defined(__EMSCRIPTEN__)
    // Parse CLI arguments into state variables
    try {
        auto args = parse_args(argc, argv);
        params = extract_params(args);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
#endif

    // Validate parameters
    if (params.initialParticles > params.maxParticles) {
        std::cerr << "Error: initialParticles > maxParticles" << std::endl;
        return 1;
    }

#ifdef __EMSCRIPTEN__
    // Initialize the Scene
    TorusParameters torus;
    SolenoidParameters solenoid;
    TokamakScene scene(torus, solenoid);
    scene.init(params);

	// Equivalent of the main loop when using Emscripten:
	auto callback = [](void *arg) {
		TokamakScene* scene = reinterpret_cast<TokamakScene*>(arg);
		scene->run_once();
	};
	emscripten_set_main_loop_arg(callback, &scene, 0, true);
#else
    // Initialize the Scene
    TorusParameters torus;
    SolenoidParameters solenoid;
    Scene* scene;
    switch (params.sceneType) {
        case SCENE_TYPE_FREE_SPACE:
            scene = new FreeSpaceScene();
            break;
        case SCENE_TYPE_TOKAMAK:
            scene = new TokamakScene(torus, solenoid);
            break;
        default:
            std::cerr << "Error: invalid scene type" << std::endl;
            return 1;
    }
    scene->init(params);
	while (scene->is_running()) {
		scene->run_once();
	}
	scene->terminate();
#endif

    return 0;
}