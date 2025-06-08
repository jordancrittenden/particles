#version 330 core
in vec3 fragNormal;
out vec4 color;

void main() {
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float brightness = max(dot(normalize(fragNormal), lightDir), 0.1);
    color = vec4(vec3(0.1, 0.7, 0.1) * brightness, 1.0); // Green color with lighting effect
}