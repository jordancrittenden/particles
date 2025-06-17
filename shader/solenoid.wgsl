struct Uniforms {
    model: mat4x4<f32>,
    view: mat4x4<f32>,
    projection: mat4x4<f32>,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
}

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) normal: vec3<f32>,
    @location(1) worldPos: vec3<f32>,
}

@vertex
fn vertexMain(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    let worldPos = uniforms.model * vec4<f32>(input.position, 1.0);
    output.position = uniforms.projection * uniforms.view * worldPos;
    output.normal = (uniforms.model * vec4<f32>(input.normal, 0.0)).xyz;
    output.worldPos = worldPos.xyz;
    return output;
}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4<f32> {
    let lightDir = normalize(vec3<f32>(1.0, 1.0, 1.0));
    let normal = normalize(input.normal);
    let diffuse = max(dot(normal, lightDir), 0.0);
    let ambient = 0.2;
    
    // Add some metallic-like specular highlights
    let viewDir = normalize(-input.worldPos);
    let reflectDir = reflect(-lightDir, normal);
    let spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    
    let baseColor = vec3<f32>(0.7, 0.7, 0.8); // Slightly bluish metallic color
    let finalColor = baseColor * (ambient + diffuse) + vec3<f32>(0.5) * spec;
    
    return vec4<f32>(finalColor, 1.0);
} 