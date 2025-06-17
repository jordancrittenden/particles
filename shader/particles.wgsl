struct VertexInput {
    @location(0) position: vec3f,
    @location(1) species: f32,
}

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
    @location(1) pointSize: f32,
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
    
    // Calculate the position in view space
    let viewPos = uniforms.view * uniforms.model * vec4f(input.position, 1.0);
    
    // Calculate the distance from the camera to the point
    let distance = length(viewPos.xyz);
    
    // Set point size based on the distance (closer points are larger)
    output.pointSize = 10.0 / (distance + 1.0); // Adjust constants as needed
    
    // Final position of the vertex in clip space
    output.position = uniforms.projection * viewPos;
    
    // Set color based on species
         if (input.species == 1.0) output.color = vec4f(0.5, 0.5, 0.5, 1.0); // neutrons are gray
    else if (input.species == 2.0) output.color = vec4f(0.0, 0.0, 1.0, 1.0); // electrons are blue
    else if (input.species == 3.0) output.color = vec4f(1.0, 0.0, 0.0, 1.0); // protons are red
    else if (input.species == 4.0) output.color = vec4f(0.0, 1.0, 0.0, 1.0); // deuterium is green
    else if (input.species == 5.0) output.color = vec4f(1.0, 0.0, 1.0, 1.0); // tritium is purple
    else if (input.species == 6.0) output.color = vec4f(1.0, 0.7, 0.0, 1.0); // helium-4 is orange
    else if (input.species == 7.0) output.color = vec4f(0.0, 0.8, 0.0, 1.0); // deuterons is light green
    else if (input.species == 8.0) output.color = vec4f(0.8, 0.0, 0.8, 1.0); // tritons is light purple
    else output.color = vec4f(1.0, 1.0, 1.0, 1.0); // default white
    
    return output;
}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4f {
    return input.color;
}
