struct UniformData {
    view: mat4x4<f32>,
    projection: mat4x4<f32>,
}

@group(0) @binding(0) var<uniform> uniforms: UniformData;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) center: vec3<f32>,
    @builtin(instance_index) instance: u32,
}

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec3<f32>,
}

@vertex
fn vertexMain(input: VertexInput) -> VertexOutput {
    // Translate the vertex to the cell center
    let world_pos = input.center + input.position;
    
    // Transform to clip space
    let model_view = uniforms.view;
    let clip_pos = uniforms.projection * model_view * vec4<f32>(world_pos, 1.0);
    
    // Generate color based on position (for debugging/visualization)
    let color = vec3<f32>(
        0.5 + 0.5 * sin(input.center.x * 10.0),
        0.5 + 0.5 * sin(input.center.y * 10.0),
        0.5 + 0.5 * sin(input.center.z * 10.0)
    );
    
    return VertexOutput(clip_pos, color);
}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4<f32> {
    return vec4<f32>(input.color, 1.0);
} 