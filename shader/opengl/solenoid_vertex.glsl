#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float flux;

out vec3 fragNormal;
out float outFlux;

void main() {
    fragNormal = mat3(transpose(inverse(model))) * aNormal; 
    outFlux = flux;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}