#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float current;

out vec3 fragNormal;
out float outCurrent;

void main() {
    fragNormal = mat3(transpose(inverse(model))) * aNormal; 
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    outCurrent = current;
}