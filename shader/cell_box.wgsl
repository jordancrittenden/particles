struct UniformData {
    view: mat4x4<f32>,
    projection: mat4x4<f32>,
}

@group(0) @binding(0) var<uniform> uniforms: UniformData;
@group(0) @binding(1) var<storage, read> visibility: array<u32>;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) center: vec3<f32>,
    @builtin(instance_index) instance: u32,
}

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec3<f32>,
    @location(1) is_visible: f32,
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
    
    // Pass visibility as a float (0.0 = invisible, 1.0 = visible)
    let is_visible = f32(visibility[input.instance]);
    
    return VertexOutput(clip_pos, color, is_visible);
}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4<f32> {
    // Use visibility to control alpha - invisible boxes are fully transparent
    let alpha = input.is_visible;
    return vec4<f32>(input.color, alpha);
} 