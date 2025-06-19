#version 330 core
in vec3 fragNormal;
in float outFlux;
out vec4 color;

void main() {
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float brightness = max(dot(normalize(fragNormal), lightDir), 0.1);

    if (outFlux != 0.0) {
        color = vec4(vec3(0.1, 0.8, 0.1) * brightness, 1.0); // Green color with lighting effect
    } else {
        color = vec4(vec3(0.6, 0.6, 0.6) * brightness, 1.0); // Light gray color with lighting effect
    }
}