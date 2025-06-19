#version 330 core
in vec3 fragNormal;
in float outCurrent;
out vec4 color;

void main() {
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float brightness = max(dot(normalize(fragNormal), lightDir), 0.1);

    if (outCurrent != 0.0) {
        color = vec4(vec3(0.3, 0.7, 1.0) * brightness, 1.0); // Blue color with lighting effect
    } else {
        color = vec4(vec3(0.6, 0.6, 0.6) * brightness, 1.0); // Light gray color with lighting effect
    }
}