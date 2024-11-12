#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 loc;
layout (location = 2) in vec4 field;

out vec3 fragColor;

uniform mat4 view;
uniform mat4 projection;

// Compute rotation matrix to map vector [0, 0, 1] onto vector b
mat3 rotationMatrix(vec3 b) {
    vec3 u = vec3(0.0, 0.0, 1.0);
    vec3 v = normalize(b);

    vec3 axis = cross(u, v);
    float cosA = dot(u, v);
    mat3 Vx = mat3(
        0.0, -axis.z, axis.y,
        axis.z, 0.0, -axis.x,
        -axis.y, axis.x, 0.0
    );

    return mat3(1.0) + Vx + (1.0 / (1.0 + cosA)) * (Vx * Vx);
}

mat4 translate(vec4 t) {
    return mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        t.x, t.y, t.z, 1.0
    );
}

void main() {
    mat3 rotation = rotationMatrix(vec3(field));
    mat4 rotation4 = mat4(rotation);
    gl_Position = projection * view * translate(loc) * rotation4 * vec4(aPos, 1.0);
    fragColor = vec3(1.0, 0.0, 1.0);
}