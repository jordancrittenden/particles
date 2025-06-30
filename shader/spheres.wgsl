struct VertexInput {
    @location(0) position: vec3f,
}

struct InstanceInput {
    @location(1) instancePosition: vec4f,
}

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
    @location(1) worldPos: vec3f,
    @location(2) normal: vec3f,
}

struct Uniforms {
    view: mat4x4f,
    projection: mat4x4f,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

@vertex
fn vertexMain(input: VertexInput, instance: InstanceInput) -> VertexOutput {
    var output: VertexOutput;
    
    // Scale the sphere vertex by a small radius
    let sphereRadius = 0.001; // Small sphere radius
    let worldPos = instance.instancePosition.xyz + input.position * sphereRadius;
    
    // Calculate the position in view space
    let viewPos = uniforms.view * vec4f(worldPos, 1.0);
    
    // Final position of the vertex in clip space
    output.position = uniforms.projection * viewPos;
    
    // Pass world position and normal for lighting
    output.worldPos = worldPos;
    output.normal = normalize(input.position);
    
    // Set color based on species (same as particles.wgsl)
         if (instance.instancePosition.w == 1.0) { output.color = vec4f(0.5, 0.5, 0.5, 1.0); } // neutrons are gray
    else if (instance.instancePosition.w == 2.0) { output.color = vec4f(0.0, 0.0, 1.0, 1.0); } // electrons are blue
    else if (instance.instancePosition.w == 3.0) { output.color = vec4f(1.0, 0.0, 0.0, 1.0); } // protons are red
    else if (instance.instancePosition.w == 4.0) { output.color = vec4f(0.0, 1.0, 0.0, 1.0); } // deuterium is green
    else if (instance.instancePosition.w == 5.0) { output.color = vec4f(1.0, 0.0, 1.0, 1.0); } // tritium is purple
    else if (instance.instancePosition.w == 6.0) { output.color = vec4f(1.0, 0.7, 0.0, 1.0); } // helium-4 is orange
    else if (instance.instancePosition.w == 7.0) { output.color = vec4f(0.0, 0.8, 0.0, 1.0); } // deuterons is light green
    else if (instance.instancePosition.w == 8.0) { output.color = vec4f(0.8, 0.0, 0.8, 1.0); } // tritons is light purple
    else { output.color = vec4f(1.0, 1.0, 1.0, 1.0); } // default white
    
    return output;
}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4f {
    // Simple lighting calculation
    let lightDir = normalize(vec3f(1.0, 1.0, 1.0));
    let diffuse = max(dot(input.normal, lightDir), 0.2);
    return input.color * vec4f(diffuse, diffuse, diffuse, 1.0);
} 