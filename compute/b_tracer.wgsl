// Physical constants
const PI: f32 = 3.14159265953;
const EPSILON_0: f32 = 8.854187817e-12;
const MU_0: f32 = 1.25663706144e-6;
const Q_E: f32 = 1.602176487e-19;
const K_E: f32 = 1.0 / (4.0 * PI * EPSILON_0);
const MU_0_OVER_4_PI: f32 = MU_0 / (4.0 * PI);
const _M: f32 = 1.0; // meter

// Particle masses
const M_ELECTRON: f32 = 9.10938188e-31;
const M_PROTON: f32 = 1.67262158e-27;
const M_NEUTRON: f32 = 1.67492716e-27;
const M_DEUTERIUM: f32 = 3.34449439e-27;
const M_TRITIUM: f32 = 5.00826721e-27;
const M_HELIUM_4_NUC: f32 = 6.64647309e-27;
const M_DEUTERON: f32 = 3.34358347e-27;
const M_TRITON: f32 = 5.00735629e-27;

// Particle species enum
const NEUTRON: f32 = 1.0;
const ELECTRON: f32 = 2.0;
const PROTON: f32 = 3.0;
const DEUTERIUM: f32 = 4.0;
const TRITIUM: f32 = 5.0;
const HELIUM_4_NUC: f32 = 6.0;
const DEUTERON: f32 = 7.0;
const TRITON: f32 = 8.0;

fn particle_charge(species: f32) -> f32 {
    if (species == NEUTRON) {
        return 0.0; // neutron
    } else if (species == ELECTRON) {
        return -Q_E; // electron
    } else if (species == PROTON) {
        return Q_E; // proton
    } else if (species == DEUTERIUM) {
        return 0.0; // deuterium
    } else if (species == TRITIUM) {
        return 0.0; // tritium
    } else if (species == HELIUM_4_NUC) {
        return 2.0 * Q_E; // helium4
    } else if (species == DEUTERON) {
        return Q_E; // ion_deuterium
    } else if (species == TRITON) {
        return Q_E; // ion_tritium
    }
    return 0.0; // error case, should never happen
}

fn compute_particle_field_contributions(
    nParticles: u32,
    particlePos: array<vec4<f32>>,
    particleVel: array<vec4<f32>>,
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

        let species = particlePos[i].w;
        if (species == 0.0) {
            continue; // inactive particle
        }

        let pos = vec3<f32>(particlePos[i].xyz);
        let vel = vec3<f32>(particleVel[i].xyz);
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

fn compute_current_field_contributions(
    currentSegments: array<vec4<f32>>,
    nCurrentSegments: u32,
    loc: vec3<f32>,
    B: ptr<function, vec3<f32>>
) {
    for (var j: u32 = 0u; j < nCurrentSegments; j++) {
        let current_x = vec3<f32>(currentSegments[j * 3u].xyz);
        let current_dx = vec3<f32>(currentSegments[j * 3u + 1u].xyz);
        let current_i = currentSegments[j * 3u + 2u].x;

        let r = loc - current_x;
        let r_mag = length(r);

        *B += MU_0_OVER_4_PI * current_i * cross(current_dx, r) / (r_mag * r_mag * r_mag);
    }
}

struct BTracerParams {
    nCurrentSegments: u32,
    enableParticleFieldContributions: u32,
    nTracers: u32,
    tracerLength: u32,
}

@group(0) @binding(0) var<storage, read> nParticles: u32;
@group(0) @binding(1) var<storage, read_write> bTracerTrails: array<vec3<f32>>;
@group(0) @binding(2) var<storage, read> particlePos: array<vec4<f32>>;
@group(0) @binding(3) var<storage, read> particleVel: array<vec4<f32>>;
@group(0) @binding(4) var<storage, read> currentSegments: array<vec4<f32>>;
@group(0) @binding(5) var<uniform> params: BTracerParams;

@compute @workgroup_size(256)
fn updateTrails(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let id = global_id.x;
    if (id >= params.nTracers) {
        return;
    }

    let traceStart = id * params.tracerLength;
    var loc = bTracerTrails[traceStart];

    for (var i: u32 = 1u; i < params.tracerLength; i++) {
        // Calculate the B field at location
        var E = vec3<f32>(0.0, 0.0, 0.0);
        var B = vec3<f32>(0.0, 0.0, 0.0);

        if (params.enableParticleFieldContributions != 0u) {
            var unused: i32 = -1;
            compute_particle_field_contributions(nParticles, particlePos, particleVel, loc, -1, &E, &B, &unused);
        }

        compute_current_field_contributions(currentSegments, params.nCurrentSegments, loc, &B);

        loc += normalize(B) * 0.005 * _M;
        bTracerTrails[traceStart + i] = loc;
    }
} 