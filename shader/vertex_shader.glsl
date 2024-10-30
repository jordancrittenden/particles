#version 330 core
layout(location = 0) in vec3 aPos;   // Position attribute
layout(location = 1) in float aCharge; // Charge attribute

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 color;

void main() {
    // Calculate the position in view space
    vec4 viewPos = view * model * vec4(aPos, 1.0);

    // Calculate the distance from the camera to the point
    float distance = length(viewPos.xyz);

    // Set point size based on the distance (closer points are larger)
    gl_PointSize = 20.0 / (distance + 1.0); // Adjust constants as needed

    // Final position of the vertex in clip space
    gl_Position = projection * view * model * vec4(aPos, 1.0);

    if (aCharge == 0.0) {
        color = vec4(0.5, 0.5, 0.5, 1.0); // neutrons are gray
    } else if (aCharge == -1.0) {
        color = vec4(0.0, 0.0, 1.0, 1.0); // electrons are blue
    } else if (aCharge == 1.0) {
        color = vec4(1.0, 0.0, 0.0, 1.0); // protons are red
    } else if (aCharge == 10.0) {
        color = vec4(0.0, 1.0, 0.0, 1.0); // axes are green
    }
}