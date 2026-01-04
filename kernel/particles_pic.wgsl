@group(0) @binding(0) var<storage, read_write> nParticles: u32;
@group(0) @binding(1) var<storage, read_write> particlePos: array<vec4<f32>>;
@group(0) @binding(2) var<storage, read_write> particleVel: array<vec4<f32>>;
@group(0) @binding(3) var<storage, read_write> eField: array<vec4<f32>>;
@group(0) @binding(4) var<storage, read_write> bField: array<vec4<f32>>;
@group(0) @binding(5) var<storage, read_write> debug: array<vec4<f32>>;
@group(0) @binding(6) var<uniform> dt: f32;
@group(0) @binding(7) var<uniform> mesh: MeshProperties;
@group(0) @binding(8) var<storage, read> cellLocation: array<vec4<f32>>;

@compute @workgroup_size(256)
// Lorentz particle push based on E and B fields interpolated from mesh
fn computeMotion(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let id = global_id.x;
    if (id >= nParticles) {
        return;
    }

    var species = particlePos[id].w;
    if (species == 0.0) {
        return; // inactive particle
    }

    // Extract properties for this particle
    let pos = vec3<f32>(particlePos[id].xyz);
    var vel = vec3<f32>(particleVel[id].xyz);
    let q_over_m = charge_to_mass_ratio(species);

    // Locate the particle in the mesh
    var neighbors: CellNeighbors = cell_neighbors(pos, &mesh);

    // Compute this particle's contribution to neighbor cell fields so that it can be subtracted out
    //var self_contribution_E: CellNeighborVectors;
    //var self_contribution_B: CellNeighborVectors;
    //compute_self_field_contribution(pos, vel, species, &neighbors, &cellLocation, &self_contribution_E, &self_contribution_B);

    // Interpolate the E and B field at particle position from the mesh
    let E_interp = interp(&mesh, &neighbors, &eField, pos);
    let B_interp = interp(&mesh, &neighbors, &bField, pos);
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
}

// Compute this particle's contribution to neighbor cell fields so that it can be subtracted out
fn compute_self_field_contribution(
    pos: vec3<f32>,               // particle position
    vel: vec3<f32>,               // particle velocity
    species: f32,                 // particle species
    neighbors: ptr<function, CellNeighbors>,
    cellLocation: ptr<storage, array<vec4<f32>>, read>,
    self_contribution_E: ptr<function, CellNeighborVectors>,  // E field
    self_contribution_B: ptr<function, CellNeighborVectors>,  // B field
) {
    compute_single_particle_field_contribution(pos, vel, species, cellLocation[u32(neighbors.xm_ym_zm)].xyz, self_contribution_E.xm_ym_zm, self_contribution_B.xm_ym_zm);
    compute_single_particle_field_contribution(pos, vel, species, cellLocation[u32(neighbors.xm_ym_zp)].xyz, self_contribution_E.xm_ym_zp, self_contribution_B.xm_ym_zp);
    compute_single_particle_field_contribution(pos, vel, species, cellLocation[u32(neighbors.xm_yp_zm)].xyz, self_contribution_E.xm_yp_zm, self_contribution_B.xm_yp_zm);
    compute_single_particle_field_contribution(pos, vel, species, cellLocation[u32(neighbors.xm_yp_zp)].xyz, self_contribution_E.xm_yp_zp, self_contribution_B.xm_yp_zp);
    compute_single_particle_field_contribution(pos, vel, species, cellLocation[u32(neighbors.xp_ym_zm)].xyz, self_contribution_E.xp_ym_zm, self_contribution_B.xp_ym_zm);
    compute_single_particle_field_contribution(pos, vel, species, cellLocation[u32(neighbors.xp_ym_zp)].xyz, self_contribution_E.xp_ym_zp, self_contribution_B.xp_ym_zp);
    compute_single_particle_field_contribution(pos, vel, species, cellLocation[u32(neighbors.xp_yp_zm)].xyz, self_contribution_E.xp_yp_zm, self_contribution_B.xp_yp_zm);
    compute_single_particle_field_contribution(pos, vel, species, cellLocation[u32(neighbors.xp_yp_zp)].xyz, self_contribution_E.xp_yp_zp, self_contribution_B.xp_yp_zp);
}