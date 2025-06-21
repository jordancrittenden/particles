#pragma once

#include <string>
#include <vector>
#include <webgpu/webgpu_cpp.h>

void poll_events(wgpu::Device& device, bool yieldToWebBrowser);
const void* read_buffer(wgpu::Device& device, wgpu::Instance& instance, const wgpu::Buffer& buffer, size_t size);

wgpu::ShaderModule create_shader_module(wgpu::Device& device, std::string shaderPath);
wgpu::ShaderModule create_shader_module(wgpu::Device& device, std::string kernelPath, std::vector<std::string> headerPaths);

// Compute shader utilities
wgpu::ComputePipeline create_compute_pipeline(wgpu::Device& device, wgpu::ShaderModule& shaderModule, const char* entryPoint);
wgpu::BindGroup create_compute_bind_group(wgpu::Device& device, wgpu::BindGroupLayout& layout, const std::vector<wgpu::BindGroupEntry>& entries);
void run_compute_pass(wgpu::Device& device, wgpu::ComputePipeline& pipeline, wgpu::BindGroup& bindGroup, uint32_t workgroupCount);