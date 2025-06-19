#version 330 core
layout(location = 0) in vec3 aPos;   // Position attribute
layout(location = 1) in vec3 aColor; // Axis color

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 color;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    color = vec4(aColor, 0.0);
}