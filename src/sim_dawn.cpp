#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <cmath>
#include <cstdlib>

#include <dawn/webgpu_cpp_print.h>
#include <webgpu/webgpu_cpp.h>
#include <glm/glm.hpp>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#else
#include <GLFW/glfw3.h>
#include <webgpu/webgpu_glfw.h>
#endif

#include "keyboard_dawn.h"
#include "args_dawn.h"
#include "state_dawn.h"
#include "scene_dawn.h"
#include "tokamak_dawn.h"

wgpu::Instance instance;
wgpu::Adapter adapter;
wgpu::Device device;
wgpu::Surface surface;
wgpu::TextureFormat format;

// Window
glm::u32 windowWidth = 1600;
glm::u32 windowHeight = 1200;
GLFWwindow* window = nullptr;

// FPS
int targetFPS = 60;

Scene* scene = nullptr;

void init_webgpu() {
    wgpu::InstanceDescriptor instanceDesc{.capabilities = {.timedWaitAnyEnable = true}};
	instance = wgpu::CreateInstance(&instanceDesc);

	wgpu::Future f1 = instance.RequestAdapter(
		nullptr,
		wgpu::CallbackMode::WaitAnyOnly,
		[](wgpu::RequestAdapterStatus status, wgpu::Adapter a, wgpu::StringView message) {
			if (status != wgpu::RequestAdapterStatus::Success) {
				std::cout << "RequestAdapter: " << message << "\n";
				exit(0);
			}
			adapter = std::move(a);
		});
	instance.WaitAny(f1, UINT64_MAX);

	wgpu::DeviceDescriptor desc{};
	desc.SetUncapturedErrorCallback([](const wgpu::Device&, wgpu::ErrorType errorType, wgpu::StringView message) {
		std::cout << "Error: " << errorType << " - message: " << message << "\n";
	});

	wgpu::Future f2 = adapter.RequestDevice(
		&desc, wgpu::CallbackMode::WaitAnyOnly,
		[](wgpu::RequestDeviceStatus status, wgpu::Device d, wgpu::StringView message) {
			if (status != wgpu::RequestDeviceStatus::Success) {
				std::cout << "RequestDevice: " << message << "\n";
				exit(0);
			}
			device = std::move(d);
		});
	instance.WaitAny(f2, UINT64_MAX);

#if defined(__EMSCRIPTEN__)
	wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector src{{.selector = "#canvas"}};
	wgpu::SurfaceDescriptor surfaceDesc{.nextInChain = &src};
	surface = instance.CreateSurface(&surfaceDesc);
#else
	if (!glfwInit()) {
		return;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(windowWidth, windowHeight, "Plasma Simulation", nullptr, nullptr);
	surface = wgpu::glfw::CreateSurfaceForWindow(instance, window);
#endif

	wgpu::SurfaceCapabilities capabilities;
	surface.GetCapabilities(adapter, &capabilities);
	format = capabilities.formats[0];

	wgpu::SurfaceConfiguration config{
		.device = device,
		.format = format,
		.width = windowWidth,
		.height = windowHeight,
		.presentMode = wgpu::PresentMode::Fifo
	};
	surface.Configure(&config);
}

void render_frame() {
	wgpu::SurfaceTexture surfaceTexture;
	surface.GetCurrentTexture(&surfaceTexture);

	wgpu::RenderPassColorAttachment attachment {
		.view = surfaceTexture.texture.CreateView(),
		.loadOp = wgpu::LoadOp::Clear,
		.storeOp = wgpu::StoreOp::Store,
		.clearValue = {1.0f, 1.0f, 1.0f, 1.0f}  // White background
	};

	wgpu::RenderPassDescriptor renderpass{
		.label = "Render Pass",
		.colorAttachmentCount = 1,
		.colorAttachments = &attachment
	};

	wgpu::CommandEncoderDescriptor encoderDesc{
		.label = "Command Encoder"
	};

	wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);
	wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);

	scene->render(device, pass, static_cast<float>(windowWidth) / static_cast<float>(windowHeight));

	pass.End();
	wgpu::CommandBuffer commands = encoder.Finish();
	device.GetQueue().Submit(1, &commands);
}

// Main function
int main(int argc, char* argv[]) {
    init_webgpu();

    TorusParameters torus;
    SolenoidParameters solenoid;
    SimulationState state;

#if !defined(__EMSCRIPTEN__)
    // Parse CLI arguments into state variables
    try {
        auto args = parse_args(argc, argv);
        extract_state_vars(args, &state, &windowWidth, &windowHeight, &targetFPS);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
#endif

    // Initialize the Scene
    scene = new TokamakScene(state, torus, solenoid);
    scene->initialize(device);

    // std::vector<CurrentVector> currents = scene->get_currents();

    // Create additional OpenCL buffers
    std::vector<glm::f32vec4> dbgBuf(state.maxParticles);
    std::vector<glm::f32vec4> cellLocations;
    for (auto& cell : state.cells) {
        cellLocations.push_back(cell.pos);
    }

    // Main render loop
    glm::u32 nParticles = state.initialParticles;
    int frameCount = 0;
    int simulationStep = 0;
    float frameTimeSec = 1.0f / (float)targetFPS;
    bool pEnableToroidalRings = state.enableToroidalRings;

#if defined(__EMSCRIPTEN__)
	emscripten_set_main_loop(render_frame, 0, false);
#else
    while (!glfwWindowShouldClose(window)) {
        auto frameStart = std::chrono::high_resolution_clock::now();

		// Process keyboard input
        process_input(window, state, scene);

        glfwPollEvents();
        render_frame();
		surface.Present();
		instance.ProcessEvents();

        // std::chrono::duration<double> frameDur;
        // do {
        //     // Update kernel args that could have changed
        //     state.toroidalI = state.enableToroidalRings ? torus.maxToroidalI : 0.0f;
        //     state.solenoidFlux = state.enableSolenoidFlux ? solenoid.maxSolenoidFlux : 0.0f;

        //     // Update current segment buffer if toroidal rings have been toggled
        //     if (pEnableToroidalRings != state.enableToroidalRings) {
        //         currents = scene->get_currents();
        //         pEnableToroidalRings = state.enableToroidalRings;
        //     }

        //     if (simulationStep % 100 == 0) {
        //         std::cout << "SIM STEP " << simulationStep << " (frame " << frameCount << ") [" << nParticles << " particles]" << std::endl;
        //     }

        //     frameDur = std::chrono::high_resolution_clock::now() - frameStart;
        //     simulationStep++;
        // } while (frameDur.count() < frameTimeSec);

        state.t += state.dt;
        frameCount++;
    }
#endif

    glfwTerminate();

    delete scene;
    
    return 0;
}
