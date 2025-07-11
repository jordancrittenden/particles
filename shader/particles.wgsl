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
    view: mat4x4f,
    projection: mat4x4f,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

@vertex
fn vertexMain(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    
    // Calculate the position in view space
    let viewPos = uniforms.view * vec4f(input.position, 1.0);
    
    // Final position of the vertex in clip space
    output.position = uniforms.projection * viewPos;
    
    // Set color based on species
         if (input.species == NEUTRON)                { output.color = vec4f(0.5, 0.5, 0.5, 1.0); } // gray
    else if (input.species == ELECTRON)               { output.color = vec4f(0.0, 0.0, 1.0, 1.0); } // blue
    else if (input.species == PROTON)                 { output.color = vec4f(1.0, 0.0, 0.0, 1.0); } // red
    else if (input.species == DEUTERIUM)              { output.color = vec4f(0.0, 1.0, 0.0, 1.0); } // green
    else if (input.species == TRITIUM)                { output.color = vec4f(1.0, 0.0, 1.0, 1.0); } // purple
    else if (input.species == HELIUM_4_NUC)           { output.color = vec4f(1.0, 0.7, 0.0, 1.0); } // orange
    else if (input.species == DEUTERON)               { output.color = vec4f(0.0, 0.8, 0.0, 1.0); } // green
    else if (input.species == TRITON)                 { output.color = vec4f(0.8, 0.0, 0.8, 1.0); } // purple
    else if (input.species == ELECTRON_MACROPARTICLE) { output.color = vec4f(0.0, 0.0, 1.0, 1.0); } // blue
    else if (input.species == PROTON_MACROPARTICLE)   { output.color = vec4f(1.0, 0.0, 0.0, 1.0); } // red
    else                                              { output.color = vec4f(1.0, 1.0, 1.0, 1.0); } // default white
    
    return output;
}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4f {
    return input.color;
}
