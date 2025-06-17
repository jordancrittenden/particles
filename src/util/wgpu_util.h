#pragma once

#include <webgpu/webgpu_cpp.h>

wgpu::ShaderModule create_shader_module(wgpu::Device& device, const char* path);