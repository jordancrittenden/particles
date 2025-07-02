#include <iostream>
#include "args.h"
#include "scene.h"
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

    if (params.initialParticles > params.maxParticles) {
        std::cerr << "Error: initialParticles > maxParticles" << std::endl;
        return 1;
    }

    // Initialize the Scene
    TorusParameters torus;
    SolenoidParameters solenoid;
    TokamakScene scene(torus, solenoid);
    // Scene scene;
    scene.init(params);

#ifdef __EMSCRIPTEN__
	// Equivalent of the main loop when using Emscripten:
	auto callback = [](void *arg) {
		TokamakScene* pScene = reinterpret_cast<TokamakScene*>(arg);
		pScene->run_once();
	};
	emscripten_set_main_loop_arg(callback, &scene, 0, true);
#else
	while (scene.is_running()) {
		scene.run_once();
	}
#endif

	scene.terminate();

    return 0;
}