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

// Charge-to-mass ratios
const Q_OVER_M_ELECTRON: f32 = -1.75882020109e11;
const Q_OVER_M_PROTON: f32 = 9.57883424534e7;
const Q_OVER_M_HELIUM_4_NUC: f32 = 2.41056642418e7;
const Q_OVER_M_DEUTERON: f32 = 4.79179449646e7;
const Q_OVER_M_TRITON: f32 = 3.19964547001e7;

const MACROPARTICLE_N: f32 = 1.0e7; // Number of particles per macroparticle

// Particle species enum
const NEUTRON:                f32 = 1.0;
const ELECTRON:               f32 = 2.0;
const PROTON:                 f32 = 3.0;
const DEUTERIUM:              f32 = 4.0;
const TRITIUM:                f32 = 5.0;
const HELIUM_4_NUC:           f32 = 6.0;
const DEUTERON:               f32 = 7.0;
const TRITON:                 f32 = 8.0;
const ELECTRON_MACROPARTICLE: f32 = 102.0;
const PROTON_MACROPARTICLE:   f32 = 103.0;

fn particle_mass(species: f32) -> f32 {
    if (species == NEUTRON) {
        return M_NEUTRON;
    } else if (species == ELECTRON) {
        return M_ELECTRON;
    } else if (species == PROTON) {
        return M_PROTON;
    } else if (species == DEUTERIUM) {
        return M_DEUTERIUM;
    } else if (species == TRITIUM) {
        return M_TRITIUM;
    } else if (species == HELIUM_4_NUC) {
        return M_HELIUM_4_NUC;
    } else if (species == DEUTERON) {
        return M_DEUTERON;
    } else if (species == TRITON) {
        return M_TRITON;
    } else if (species == ELECTRON_MACROPARTICLE) {
        return M_ELECTRON * MACROPARTICLE_N;
    } else if (species == PROTON_MACROPARTICLE) {
        return M_PROTON * MACROPARTICLE_N;
    }
    return 0.0; // error case, should never happen
}

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
    } else if (species == ELECTRON_MACROPARTICLE) {
        return -Q_E * MACROPARTICLE_N;
    } else if (species == PROTON_MACROPARTICLE) {
        return  Q_E * MACROPARTICLE_N;
    }
    return 0.0; // error case, should never happen
}

fn charge_to_mass_ratio(species: f32) -> f32 {
    if (species == NEUTRON) {
        return 0.0;
    } else if (species == ELECTRON) {
        return Q_OVER_M_ELECTRON;
    } else if (species == PROTON) {
        return Q_OVER_M_PROTON;
    } else if (species == DEUTERIUM) {
        return 0.0;
    } else if (species == TRITIUM) {
        return 0.0;
    } else if (species == HELIUM_4_NUC) {
        return Q_OVER_M_HELIUM_4_NUC;
    } else if (species == DEUTERON) {
        return Q_OVER_M_DEUTERON;
    } else if (species == TRITON) {
        return Q_OVER_M_TRITON;
    } else if (species == ELECTRON_MACROPARTICLE) {
        return Q_OVER_M_ELECTRON;
    } else if (species == PROTON_MACROPARTICLE) {
        return Q_OVER_M_PROTON;
    }
    return 0.0; // error case, should never happen
}