struct VertexInput {
    @location(0) position: vec3f,
    @location(1) color: vec3f,
}

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
}

struct Uniforms {
    model: mat4x4f,
    view: mat4x4f,
    projection: mat4x4f,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

@vertex
fn vertexMain(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    output.position = uniforms.projection * uniforms.view * uniforms.model * vec4f(input.position, 1.0);
    output.color = vec4f(input.color, 0.0);
    return output;
}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4f {
    return input.color;
}
