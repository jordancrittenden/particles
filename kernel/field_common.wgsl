fn compute_particle_field_contributions(
    nParticles: u32,
    particlePos: ptr<storage, array<vec4<f32>>, read_write>,
    particleVel: ptr<storage, array<vec4<f32>>, read_write>,
    loc: vec3<f32>,
    skipId: i32,
    E: ptr<function, vec3<f32>>,
    B: ptr<function, vec3<f32>>,
    colliderId: ptr<function, i32>
) {
    for (var i: u32 = 0u; i < nParticles; i++) {
        if (i == u32(skipId)) {
            continue;
        }

        let species = (*particlePos)[i].w;
        if (species == 0.0) {
            continue; // inactive particle
        }

        let pos = vec3<f32>((*particlePos)[i].xyz);
        let vel = vec3<f32>((*particleVel)[i].xyz);
        let charge = particle_charge(species);

        let r = loc - pos;
        let r_norm = normalize(r);
        let r_mag = length(r);

        // Avoid division by zero
        if (r_mag < 0.00001) {
            *colliderId = i32(i);
            continue;
        } else {
            *E += ((K_E * charge) / (r_mag * r_mag)) * r_norm;
            *B += ((MU_0_OVER_4_PI * charge) / (r_mag * r_mag)) * cross(vel, r_norm);
        }
    }
}

fn compute_currents_b_field(
    currentSegments: ptr<storage, array<vec4<f32>>, read>,
    nCurrentSegments: u32,
    loc: vec3<f32>
) -> vec3<f32> {
    var B = vec3<f32>(0.0, 0.0, 0.0);
    for (var j: u32 = 0u; j < nCurrentSegments; j++) {
        let current_x = vec3<f32>((*currentSegments)[j * 3u].xyz);
        let current_dx = vec3<f32>((*currentSegments)[j * 3u + 1u].xyz);
        let current_i = (*currentSegments)[j * 3u + 2u].x;
        if (current_i == 0.0) { continue; }

        let r = loc - current_x;
        let r_mag = length(r);

        B += MU_0_OVER_4_PI * current_i * cross(current_dx, r) / (r_mag * r_mag * r_mag);
    }
    return B;
}

fn compute_solenoid_e_field(
    solenoidFlux: f32,
    loc: vec3<f32>,
) -> vec3<f32> {
    let solenoid_axis = vec3<f32>(0.0, 1.0, 0.0);
    let solenoid_r = vec3<f32>(loc.x, 0.0, loc.z);
    let solenoid_e_mag = solenoidFlux / (2.0 * PI * length(solenoid_r));
    return solenoid_e_mag * cross(solenoid_axis, normalize(solenoid_r));
} 