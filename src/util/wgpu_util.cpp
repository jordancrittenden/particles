#include <webgpu/webgpu_cpp.h>
#include <fstream>
#include <sstream>
#include <iostream>

std::string load_shader_source(const char* filepath) {
    std::ifstream file;
    std::stringstream buffer;

    // Ensure ifstream objects can throw exceptions
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        file.open(filepath);
        buffer << file.rdbuf();  // Read the entire file into the buffer
        file.close();
    } catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << filepath << std::endl;
    }

    return buffer.str();  // Return the content as a string
}

wgpu::ShaderModule create_shader_module(wgpu::Device& device, const char* path) {
    std::string shaderCode = load_shader_source(path);
    wgpu::ShaderSourceWGSL wgsl{{.code = shaderCode.c_str()}};
    wgpu::ShaderModuleDescriptor shaderModuleDescriptor{.nextInChain = &wgsl};
    return device.CreateShaderModule(&shaderModuleDescriptor);
}