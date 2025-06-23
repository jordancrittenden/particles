struct Uniforms {
    view: mat4x4<f32>,
    projection: mat4x4<f32>,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) location: vec4<f32>,  // Instance data: position + field type (0=E, 1=B)
    @location(2) field: vec4<f32>,     // Instance data: field vector
}

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) fieldType: f32,       // 0.0 for E field, 1.0 for B field
}

// Compute rotation matrix to map vector [0, 0, 1] onto vector b
fn rotationMatrix(b: vec3<f32>) -> mat3x3<f32> {
    let u = normalize(b);
    let v = vec3<f32>(0.0, 0.0, 1.0);

    let axis = cross(u, v);
    let cosA = dot(u, v);
    let Vx = mat3x3<f32>(
        vec3<f32>(0.0, -axis.z, axis.y),
        vec3<f32>(axis.z, 0.0, -axis.x),
        vec3<f32>(-axis.y, axis.x, 0.0)
    );

    let I = mat3x3<f32>(
        vec3<f32>(1.0, 0.0, 0.0),
        vec3<f32>(0.0, 1.0, 0.0),
        vec3<f32>(0.0, 0.0, 1.0)
    );

    return I + Vx + (1.0 / (1.0 + cosA)) * (Vx * Vx);
}

fn translate(t: vec4<f32>) -> mat4x4<f32> {
    return mat4x4<f32>(
        vec4<f32>(1.0, 0.0, 0.0, 0.0),
        vec4<f32>(0.0, 1.0, 0.0, 0.0),
        vec4<f32>(0.0, 0.0, 1.0, 0.0),
        vec4<f32>(t.x, t.y, t.z, 1.0)
    );
}

@vertex
fn vertexMain(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    
    let rotation = rotationMatrix(input.field.xyz);
    let rotation4 = mat4x4<f32>(
        vec4<f32>(rotation[0], 0.0),
        vec4<f32>(rotation[1], 0.0),
        vec4<f32>(rotation[2], 0.0),
        vec4<f32>(0.0, 0.0, 0.0, 1.0)
    );
    
    let worldPos = translate(input.location) * rotation4 * vec4<f32>(input.position, 1.0);
    output.position = uniforms.projection * uniforms.view * worldPos;
    output.fieldType = input.location.w;  // 0.0 for E field, 1.0 for B field
    
    return output;
}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4<f32> {
    // Color based on field type: E field = red, B field = blue
    let color = select(
        vec3<f32>(1.0, 0.0, 0.0),  // Red for E field (input.fieldType == 0.0)
        vec3<f32>(0.0, 0.0, 1.0),  // Blue for B field (input.fieldType == 1.0)
        input.fieldType > 0.5
    );
    
    return vec4<f32>(color, 1.0);
} 