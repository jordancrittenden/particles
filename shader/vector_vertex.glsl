#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in mat4 instanceMatrix;

out vec3 fragColor;

uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * instanceMatrix * vec4(aPos, 1.0);
    fragColor = vec3(0.0, 0.0, 0.0);
}