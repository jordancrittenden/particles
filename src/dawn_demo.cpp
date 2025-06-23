#include <iostream>
#include <glm/glm.hpp>
#include <dawn/webgpu_cpp_print.h>
#include <webgpu/webgpu_cpp.h>

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
wgpu::RenderPipeline pipeline;
wgpu::Surface surface;
wgpu::TextureFormat format;
const uint32_t kWidth = 512;
const uint32_t kHeight = 512;

const char shaderCode[] = R"(
	@vertex fn vertexMain(@builtin(vertex_index) i : u32) ->
		@builtin(position) vec4f {
		const pos = array(vec2f(0, 1), vec2f(-1, -1), vec2f(1, -1));
		return vec4f(pos[i], 0, 1);
	}
	@fragment fn fragmentMain() -> @location(0) vec4f {
		return vec4f(1, 0, 1, 1);
	}
)";

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

void render_frame() {
	wgpu::SurfaceTexture surfaceTexture;
	surface.GetCurrentTexture(&surfaceTexture);

	wgpu::RenderPassColorAttachment attachment {
		.view = surfaceTexture.texture.CreateView(),
		.loadOp = wgpu::LoadOp::Clear,
		.storeOp = wgpu::StoreOp::Store,
		.clearValue = {1.0f, 1.0f, 1.0f, 1.0f}  // White background
	};

	wgpu::TextureDescriptor depthTextureDesc {
		.size = {kWidth, kHeight},
		.format = wgpu::TextureFormat::Depth24Plus,
		.usage = wgpu::TextureUsage::RenderAttachment
	};
	wgpu::Texture depthTexture = device.CreateTexture(&depthTextureDesc);

	wgpu::RenderPassDepthStencilAttachment depthAttachment {
		.view = depthTexture.CreateView(),
		.depthClearValue = 1.0,
		.depthLoadOp = wgpu::LoadOp::Clear,
		.depthStoreOp = wgpu::StoreOp::Store,
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

	pass.SetPipeline(pipeline);
	pass.Draw(3);

	pass.End();
	wgpu::CommandBuffer commands = encoder.Finish();
	device.GetQueue().Submit(1, &commands);
}

int main() {
	init_webgpu(kWidth, kHeight);

	wgpu::ShaderSourceWGSL wgsl{{.code = shaderCode}};
	wgpu::ShaderModuleDescriptor shaderModuleDescriptor{.nextInChain = &wgsl};
	wgpu::ShaderModule shaderModule =  device.CreateShaderModule(&shaderModuleDescriptor);
	wgpu::ColorTargetState colorTargetState{.format = format};
	wgpu::FragmentState fragmentState{
		.module = shaderModule,
		.targetCount = 1,
		.targets = &colorTargetState
	};
    wgpu::DepthStencilState depthStencilState = {
        .depthWriteEnabled = true,
        .depthCompare = wgpu::CompareFunction::Less,
        .format = wgpu::TextureFormat::Depth24Plus
    };
	wgpu::RenderPipelineDescriptor descriptor{
		.vertex = {.module = shaderModule},
		.fragment = &fragmentState,
		.depthStencil = &depthStencilState
	};

	pipeline = device.CreateRenderPipeline(&descriptor);

#if defined(__EMSCRIPTEN__)
	emscripten_set_main_loop(render_frame, 0, false);
#else
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		render_frame();
		surface.Present();
		instance.ProcessEvents();
	}
#endif

	return 0;
}