#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 instancePos;
layout (location = 2) in vec3 instanceDir;
layout (location = 3) in vec3 instanceColor;

out vec3 fragColor;

uniform mat4 view;
uniform mat4 projection;
uniform float length;

mat4 translate(vec3 translation) {
    return mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        translation.x, translation.y, translation.z, 1.0
    );
}

mat4 scale(vec3 scaling) {
    return mat4(
        scaling.x, 0.0, 0.0, 0.0,
        0.0, scaling.y, 0.0, 0.0,
        0.0, 0.0, scaling.z, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}

void main() {
    vec3 axisZ = normalize(instanceDir);
    vec3 axisY = vec3(0.0, 1.0, 0.0);
    vec3 axisX = cross(axisY, axisZ);

    mat4 rotation = mat4(
        vec4(axisX, 0.0),
        vec4(axisY, 0.0),
        vec4(axisZ, 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    );

    mat4 model = translate(instancePos) * rotation * scale(vec3(1.0, 1.0, length));

    gl_Position = projection * view * model * vec4(aPos, 1.0);
    fragColor = instanceColor;
}