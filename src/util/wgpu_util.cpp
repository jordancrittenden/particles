#include <webgpu/webgpu_cpp.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>
#include <cstdio>
#include "wgpu_util.h"

// We define a function that hides implementation-specific variants of device polling
void poll_events(wgpu::Device& device, bool yieldToWebBrowser) {
#if defined(WEBGPU_BACKEND_DAWN)
    device.tick();
#elif defined(WEBGPU_BACKEND_WGPU)
    device.poll(false);
#elif defined(WEBGPU_BACKEND_EMSCRIPTEN)
    if (yieldToWebBrowser) {
        emscripten_sleep(100);
    }
#endif
}

const void* read_buffer(wgpu::Device& device, wgpu::Instance& instance, const wgpu::Buffer& buffer, size_t size) {
    bool success = false;
    wgpu::FutureWaitInfo waitInfo {
        buffer.MapAsync(
            wgpu::MapMode::Read,
            0,
            size,
            wgpu::CallbackMode::WaitAnyOnly,
            [&](wgpu::MapAsyncStatus status, wgpu::StringView message) {
                success = status == wgpu::MapAsyncStatus::Success;
                if (!success) {
                    std::cerr << "Error reading buffer: " << message.data << std::endl;
                }}
            )
    };
    wgpu::WaitStatus status =
        instance.WaitAny(1, &waitInfo, std::numeric_limits<uint64_t>::max());
    if (!success) {
        return nullptr;
    }
    const void* data = buffer.GetConstMappedRange(0, size);
    buffer.Unmap();
    return data;
}

std::string read_file(std::string path) {
    std::string buf;
#if defined(__EMSCRIPTEN__)
    // Use Emscripten's file system API for web
    FILE* file = fopen(path.c_str(), "r");
    if (!file) {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return nullptr;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Read file content
    buf.resize(fileSize);
    fread(&buf[0], 1, fileSize, file);
    fclose(file);
#else
    // Use standard file I/O for native platforms
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return nullptr;
    }
    buf = std::string(std::istreambuf_iterator<char>(file), (std::istreambuf_iterator<char>()));
    file.close();
#endif
    return buf;
}

wgpu::ShaderModule create_shader_module(wgpu::Device& device, std::string path, std::vector<std::string> headerPaths) {
    // Load shader source
    std::string shaderSrc = read_file(path);

    // Load header sources
    // WebGPU does not support #include, so we manually support including header files
    std::string headerSrc;
    for (const auto& headerPath : headerPaths) {
        headerSrc += read_file(headerPath);
        headerSrc += "\n";
    }

    std::string src = headerSrc + "\n" + shaderSrc;
    std::cout << "Creating shader module for: " << path << " (size: " << src.size() << " chars)" << std::endl;
    
    wgpu::ShaderSourceWGSL wgsl{{.code = src.c_str()}};
    wgpu::ShaderModuleDescriptor shaderModuleDescriptor{.label = path.c_str(), .nextInChain = &wgsl};
    return device.CreateShaderModule(&shaderModuleDescriptor);
}

wgpu::ShaderModule create_shader_module(wgpu::Device& device, std::string path) {
    return create_shader_module(device, path, {});
}

wgpu::ComputePipeline create_compute_pipeline(wgpu::Device& device, wgpu::ShaderModule& shaderModule, const char* entryPoint) {
    wgpu::ComputePipelineDescriptor computePipelineDesc = {
        .label = "Compute Pipeline",
        .layout = nullptr,  // Auto layout
        .compute = {
            .module = shaderModule,
            .entryPoint = entryPoint
        }
    };
    return device.CreateComputePipeline(&computePipelineDesc);
}

wgpu::BindGroup create_compute_bind_group(wgpu::Device& device, wgpu::BindGroupLayout& layout, const std::vector<wgpu::BindGroupEntry>& entries) {
    wgpu::BindGroupDescriptor bindGroupDesc = {
        .layout = layout,
        .entryCount = static_cast<uint32_t>(entries.size()),
        .entries = entries.data()
    };
    return device.CreateBindGroup(&bindGroupDesc);
}

void run_compute_pass(wgpu::Device& device, wgpu::ComputePipeline& pipeline, wgpu::BindGroup& bindGroup, uint32_t workgroupCount) {
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ComputePassEncoder computePass = encoder.BeginComputePass();
    
    computePass.SetPipeline(pipeline);
    computePass.SetBindGroup(0, bindGroup);
    computePass.DispatchWorkgroups(workgroupCount, 1, 1);
    
    computePass.End();
    wgpu::CommandBuffer commands = encoder.Finish();
    device.GetQueue().Submit(1, &commands);
}