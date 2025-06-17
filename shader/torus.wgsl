struct Uniforms {
    model: mat4x4<f32>,
    view: mat4x4<f32>,
    projection: mat4x4<f32>,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) instanceMatrix0: vec4<f32>,
    @location(3) instanceMatrix1: vec4<f32>,
    @location(4) instanceMatrix2: vec4<f32>,
    @location(5) instanceMatrix3: vec4<f32>,
}

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) normal: vec3<f32>,
}

@vertex
fn vertexMain(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    
    // Construct instance matrix
    let instanceMatrix = mat4x4<f32>(
        input.instanceMatrix0,
        input.instanceMatrix1,
        input.instanceMatrix2,
        input.instanceMatrix3
    );
    
    // Transform position
    var worldPos = uniforms.model * instanceMatrix * vec4<f32>(input.position, 1.0);
    output.position = uniforms.projection * uniforms.view * worldPos;
    
    // Transform normal
    output.normal = (uniforms.model * instanceMatrix * vec4<f32>(input.normal, 0.0)).xyz;
    
    return output;
}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4<f32> {
    // Simple lighting
    let lightDir = normalize(vec3<f32>(1.0, 1.0, 1.0));
    let normal = normalize(input.normal);
    let diffuse = max(dot(normal, lightDir), 0.0);
    let ambient = 0.2;
    let color = vec3<f32>(0.8, 0.8, 0.8); // Light gray color
    let finalColor = color * (ambient + diffuse);
    
    return vec4<f32>(finalColor, 1.0);
} 