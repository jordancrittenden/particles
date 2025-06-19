struct Uniforms {
    view: mat4x4<f32>,
    projection: mat4x4<f32>,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) modelMatrix0: vec4<f32>,
    @location(3) modelMatrix1: vec4<f32>,
    @location(4) modelMatrix2: vec4<f32>,
    @location(5) modelMatrix3: vec4<f32>,
}

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) normal: vec3<f32>,
    @location(1) worldPos: vec3<f32>,
}

@vertex
fn vertexMain(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    
    // Reconstruct model matrix from vertex attributes
    let modelMatrix = mat4x4<f32>(
        input.modelMatrix0,
        input.modelMatrix1,
        input.modelMatrix2,
        input.modelMatrix3
    );
    
    let worldPos = modelMatrix * vec4<f32>(input.position, 1.0);
    output.position = uniforms.projection * uniforms.view * worldPos;
    output.normal = (modelMatrix * vec4<f32>(input.normal, 0.0)).xyz;
    output.worldPos = worldPos.xyz;
    return output;
}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4<f32> {
    let lightDir = normalize(vec3<f32>(1.0, 1.0, 1.0));
    let brightness = max(dot(normalize(input.normal), lightDir), 0.1);
    
    return vec4(vec3(0.6, 0.6, 0.6) * brightness, 1.0);
} 