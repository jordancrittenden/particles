struct Uniforms {
    view: mat4x4<f32>,
    projection: mat4x4<f32>,
    color: vec4f,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

struct VertexInput {
    @location(0) position: vec3<f32>,
}

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
}

@vertex
fn vertexMain(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;

    // Calculate the position in view space
    let viewPos = uniforms.view * vec4f(input.position, 1.0);

    // Final position of the vertex in clip space
    output.position = uniforms.projection * viewPos;

    return output;
}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4<f32> {
    return uniforms.color;
} 