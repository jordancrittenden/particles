#pragma once

#include <string>
#include <vector>
#include <webgpu/webgpu_cpp.h>

wgpu::ShaderModule create_shader_module(wgpu::Device& device, std::string shaderPath);
wgpu::ShaderModule create_shader_module(wgpu::Device& device, std::string kernelPath, std::vector<std::string> headerPaths);

// Compute shader utilities
wgpu::ComputePipeline create_compute_pipeline(wgpu::Device& device, wgpu::ShaderModule& shaderModule, const char* entryPoint);
wgpu::BindGroup create_compute_bind_group(wgpu::Device& device, wgpu::BindGroupLayout& layout, const std::vector<wgpu::BindGroupEntry>& entries);
void run_compute_pass(wgpu::Device& device, wgpu::ComputePipeline& pipeline, wgpu::BindGroup& bindGroup, uint32_t workgroupCount);