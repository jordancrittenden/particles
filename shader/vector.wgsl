struct Uniforms {
    model: mat4x4<f32>,
    view: mat4x4<f32>,
    projection: mat4x4<f32>,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) direction: vec3<f32>,
    @location(2) magnitude: f32,
}

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) direction: vec3<f32>,
    @location(1) magnitude: f32,
}

@vertex
fn vertexMain(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    let worldPos = uniforms.model * vec4<f32>(input.position, 1.0);
    output.position = uniforms.projection * uniforms.view * worldPos;
    output.direction = (uniforms.model * vec4<f32>(input.direction, 0.0)).xyz;
    output.magnitude = input.magnitude;
    return output;
}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4<f32> {
    // Color mapping based on magnitude
    let normalizedMag = clamp(input.magnitude, 0.0, 1.0);
    
    // Blue to red color gradient
    let color = vec3<f32>(
        normalizedMag,                    // Red component
        0.0,                             // Green component
        1.0 - normalizedMag              // Blue component
    );
    
    return vec4<f32>(color, 1.0);
} 