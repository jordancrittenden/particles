struct Uniforms {
    model: mat4x4<f32>,
    view: mat4x4<f32>,
    projection: mat4x4<f32>,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) color: vec4<f32>,
    @location(2) age: f32,
}

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec4<f32>,
    @location(1) age: f32,
}

@vertex
fn vertexMain(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    let worldPos = uniforms.model * vec4<f32>(input.position, 1.0);
    output.position = uniforms.projection * uniforms.view * worldPos;
    output.color = input.color;
    output.age = input.age;
    return output;
}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4<f32> {
    // Fade out based on age
    let alpha = 1.0 - input.age;
    return vec4<f32>(input.color.rgb, input.color.a * alpha);
} 