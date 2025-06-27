struct Uniforms {
    view: mat4x4<f32>,
    projection: mat4x4<f32>,
    color: vec4f,
    headIdx: u32,
    tracerLength: u32,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

struct VertexInput {
    @builtin(vertex_index) idx: u32,
    @location(0) position: vec3<f32>,
}

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) alpha: f32,
}

@vertex
fn vertexMain(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;

    // Calculate the position in view space
    let viewPos = uniforms.view * vec4f(input.position, 1.0);

    // Final position of the vertex in clip space
    output.position = uniforms.projection * viewPos;

    let distFromHead = ((uniforms.headIdx - (input.idx % uniforms.tracerLength)) % uniforms.tracerLength);
    output.alpha = 1.0 - (f32(distFromHead) / f32(uniforms.tracerLength));

    return output;
}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4<f32> {
    return vec4(uniforms.color.x, uniforms.color.y, uniforms.color.z, input.alpha);
} 