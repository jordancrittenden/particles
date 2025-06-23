#include <iostream>
#include <chrono>
#include <glm/glm.hpp>

#include <dawn/webgpu_cpp_print.h>
#include <webgpu/webgpu_cpp.h>

#include "args.h"
#include "scene.h"
#include "tokamak.h"
#include "render/particles.h"
#include "compute/particles.h"
#include "current_segment.h"
#include "plasma.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#else
#include <GLFW/glfw3.h>
#include <webgpu/webgpu_glfw.h>

GLFWwindow* window = nullptr;
#endif

wgpu::Instance instance;
wgpu::Adapter adapter;
wgpu::Device device;
wgpu::Surface surface;
wgpu::TextureFormat format;

SimulationParams params;

auto lastInputTime = std::chrono::high_resolution_clock::now();
bool debounce_input() {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = now - lastInputTime;
    if (diff.count() < 0.1) return false;
    lastInputTime = now;
    return true;
}

void init_webgpu(glm::u32 windowWidth, glm::u32 windowHeight) {
    wgpu::InstanceDescriptor instanceDesc{.capabilities = {.timedWaitAnyEnable = true}};
    instance = wgpu::CreateInstance(&instanceDesc);

    wgpu::Future f1 = instance.RequestAdapter(
        nullptr,
        wgpu::CallbackMode::WaitAnyOnly,
        [](wgpu::RequestAdapterStatus status, wgpu::Adapter a, wgpu::StringView message) {
            if (status != wgpu::RequestAdapterStatus::Success) {
                std::cout << "RequestAdapter: " << message.data << "\n";
                exit(0);
            }
            adapter = std::move(a);
        });
    instance.WaitAny(f1, UINT64_MAX);

    wgpu::DeviceDescriptor desc{};
    desc.SetUncapturedErrorCallback([](const wgpu::Device&, wgpu::ErrorType errorType, wgpu::StringView message) {
        std::cout << "Error: " << static_cast<uint32_t>(errorType) << " - message: " << message.data << "\n";
    });

    wgpu::Future f2 = adapter.RequestDevice(
        &desc, wgpu::CallbackMode::WaitAnyOnly,
        [](wgpu::RequestDeviceStatus status, wgpu::Device d, wgpu::StringView message) {
            if (status != wgpu::RequestDeviceStatus::Success) {
                std::cout << "RequestDevice: " << message.data << "\n";
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

void render_frame(void* scene) {
    wgpu::SurfaceTexture surfaceTexture;
    surface.GetCurrentTexture(&surfaceTexture);

    wgpu::RenderPassColorAttachment attachment {
        .view = surfaceTexture.texture.CreateView(),
        .loadOp = wgpu::LoadOp::Clear,
        .storeOp = wgpu::StoreOp::Store,
        .clearValue = {1.0f, 1.0f, 1.0f, 1.0f}  // White background
    };

    wgpu::TextureDescriptor depthTextureDesc {
        .usage = wgpu::TextureUsage::RenderAttachment,
        .size = {params.windowWidth, params.windowHeight},
        .format = wgpu::TextureFormat::Depth24Plus
    };
    wgpu::Texture depthTexture = device.CreateTexture(&depthTextureDesc);

    wgpu::RenderPassDepthStencilAttachment depthAttachment {
        .view = depthTexture.CreateView(),
        .depthLoadOp = wgpu::LoadOp::Clear,
        .depthStoreOp = wgpu::StoreOp::Store,
        .depthClearValue = 1.0
    };

    wgpu::RenderPassDescriptor renderpass{
        .label = "Render Pass",
        .colorAttachmentCount = 1,
        .colorAttachments = &attachment,
        .depthStencilAttachment = &depthAttachment
    };

    wgpu::CommandEncoderDescriptor encoderDesc{.label = "Render Command Encoder"};
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);

    ((Scene*)scene)->render(device, pass, static_cast<float>(params.windowWidth) / static_cast<float>(params.windowHeight));

    pass.End();
    wgpu::CommandBuffer commands = encoder.Finish();
    device.GetQueue().Submit(1, &commands);
}

// Main function
int main(int argc, char* argv[]) {
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

    // Initialize WebGPU
    init_webgpu(params.windowWidth, params.windowHeight);

    // Initialize the Scene
    TorusParameters torus;
    SolenoidParameters solenoid;
    Scene* scene = new TokamakScene(torus, solenoid);
    scene->initialize(device, params);

    // Main render loop
    int frameCount = 0;
    int simulationStep = 0;
    float frameTimeSec = 1.0f / (float)params.targetFPS;

#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop_arg(render_frame, scene, 0, false);
#else
    while (!glfwWindowShouldClose(window)) {
        auto frameStart = std::chrono::high_resolution_clock::now();

        // Process keyboard input
        glfwPollEvents();
        scene->process_input(window, debounce_input);
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        // Render frame
        render_frame(scene);
        surface.Present();
        instance.ProcessEvents();

        std::chrono::duration<double> frameDur;
        do {
            // Run compute pass
            wgpu::CommandEncoderDescriptor encoderDesc{.label = "Compute Command Encoder"};
            wgpu::CommandEncoder encoder = device.CreateCommandEncoder(&encoderDesc);
            wgpu::ComputePassDescriptor computePassDesc{.label = "Compute Pass"};
            wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&computePassDesc);
            scene->compute_step(device, pass);
            pass.End();
            scene->compute_copy(encoder);
            wgpu::CommandBuffer commands = encoder.Finish();
            device.GetQueue().Submit(1, &commands);
            scene->compute_read(device, instance);

            if (simulationStep % 100 == 0) {
                std::cout << "SIM STEP " << simulationStep << " (frame " << frameCount << ") [" << scene->getNumParticles() << " particles]" << std::endl;
            }

            frameDur = std::chrono::high_resolution_clock::now() - frameStart;
            simulationStep++;
        } while (frameDur.count() < frameTimeSec);

        frameCount++;
    }
#endif

#if !defined(__EMSCRIPTEN__)
    glfwTerminate();
#endif

    delete scene;
    
    return 0;
}