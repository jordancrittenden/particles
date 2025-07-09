// Constants for particle constraints
const CONSTRAIN: bool = false;
const CONSTRAIN_TO: f32 = 0.1;  // meters

@group(0) @binding(0) var<storage, read_write> nParticles: u32;
@group(0) @binding(1) var<storage, read_write> particlePos: array<vec4<f32>>;
@group(0) @binding(2) var<storage, read_write> particleVel: array<vec4<f32>>;
@group(0) @binding(3) var<storage, read_write> eField: array<vec4<f32>>;
@group(0) @binding(4) var<storage, read_write> bField: array<vec4<f32>>;
@group(0) @binding(5) var<storage, read_write> debug: array<vec4<f32>>;
@group(0) @binding(6) var<uniform> dt: f32;
@group(0) @binding(7) var<uniform> mesh: MeshProperties;

@compute @workgroup_size(256)
fn computeMotion(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let id = global_id.x;
    if (id >= nParticles) {
        return;
    }

    let species = particlePos[id].w;
    if (species == 0.0) {
        return; // inactive particle
    }

    // Extract properties for this particle
    let pos = vec3<f32>(particlePos[id].xyz);
    let vel = vec3<f32>(particleVel[id].xyz);
    let mass = particle_mass(species);
    let q_over_m = charge_to_mass_ratio(species);

    // Interpolate the E and B field at particle position from the mesh
    let E_interp = interp(&mesh, &eField, pos);
    let B_interp = interp(&mesh, &bField, pos);
    let E = vec3<f32>(E_interp.x, E_interp.y, E_interp.z);
    let B = vec3<f32>(B_interp.x, B_interp.y, B_interp.z);

    // Push the particle through the electric and magnetic field: dv/dt = q/m (E + v x B);
    let t = q_over_m * B * 0.5 * dt;
    let s = 2.0 * t / (1.0 + (length(t) * length(t)));
    let v_minus = vel + q_over_m * E * 0.5 * dt;
    let v_prime = v_minus + cross(v_minus, t);
    let v_plus = v_minus + cross(v_prime, s);
    let vel_new = v_plus + (q_over_m * E * 0.5 * dt);
    let pos_new = pos + (vel_new * dt);

    particlePos[id] = vec4<f32>(pos_new, species);
    particleVel[id] = vec4<f32>(vel_new, 0.0);

    //debug[id] = vec4<f32>(0.0, 0.0, 0.0, 0.0);

    if (CONSTRAIN) {
        // Keep the particles in their box
        if (particlePos[id].x > CONSTRAIN_TO) {
            particleVel[id].x = -particleVel[id].x;
        }
        if (particlePos[id].x < -CONSTRAIN_TO) {
            particleVel[id].x = -particleVel[id].x;
        }
        if (particlePos[id].y > CONSTRAIN_TO) {
            particleVel[id].y = -particleVel[id].y;
        }
        if (particlePos[id].y < -CONSTRAIN_TO) {
            particleVel[id].y = -particleVel[id].y;
        }
        if (particlePos[id].z > CONSTRAIN_TO) {
            particleVel[id].z = -particleVel[id].z;
        }
        if (particlePos[id].z < -CONSTRAIN_TO) {
            particleVel[id].z = -particleVel[id].z;
        }
    }
} 