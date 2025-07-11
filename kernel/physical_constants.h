// Header to be included in C++ and kernels
#ifndef _PHYSICAL_CONSTANTS_H_
#define _PHYSICAL_CONSTANTS_H_

#define _KG                   (1.0f) // kilogram
#define _M                    (1.0f) // meter
#define _S                    (1.0f) // second
#define _A                    (1.0f) // ampere
#define _K                    (1.0f) // kelvin
#define _V                    (_KG * _M * _M / (_A * _S * _S * _S)) // volt
#define _J                    (_KG * _M * _M / (_S * _S)) // joule

#define PI                    (3.14159265953f)

#define M_ELECTRON            (9.10938188e-31f * _KG)    /* kg */
#define M_PROTON              (1.67262158e-27f * _KG)    /* kg */
#define M_NEUTRON             (1.67492716e-27f * _KG)    /* kg */
#define M_DEUTERIUM           (3.34449439e-27f * _KG)    /* kg */
#define M_TRITIUM             (5.00826721e-27f * _KG)    /* kg */
#define M_HELIUM_4_NUC        (6.64647309e-27f * _KG)    /* kg */
#define M_DEUTERON            (3.34358347e-27f * _KG)    /* kg */
#define M_TRITON              (5.00735629e-27f * _KG)    /* kg */

#define EPSILON_0             (8.854187817e-12f               * (_A * _S * _S / (_KG * _M * _M * _M)))  /* A^2 s^4 / kg m^3 */
#define MU_0                  (1.25663706144e-6f              * (_KG * _M / (_A * _A * _S * _S)))       /* kg m / A^2 s^2 */
#define Q_E                   (1.602176487e-19f               * (_A * _S))                              /* A s */
#define K_E                   (1.0f / (4.0f * PI * EPSILON_0) * (_KG * _M / (_A * _A * _S * _S)))       /* kg m^3 / A^2 s^4 */
#define MU_0_OVER_4_PI        (MU_0 / (4.0f * PI)             * (_KG * _M / (_A * _A * _S * _S)))       /* kg m / A^2 s^2 */

#define Q_OVER_M_ELECTRON     (-1.75882020109e11f * (_A * _S / _KG))    /* A s / kg */
#define Q_OVER_M_PROTON       ( 9.57883424534e7f  * (_A * _S / _KG))    /* A s / kg */
#define Q_OVER_M_HELIUM_4_NUC ( 2.41056642418e7f  * (_A * _S / _KG))    /* A s / kg */
#define Q_OVER_M_DEUTERON     ( 4.79179449646e7f  * (_A * _S / _KG))    /* A s / kg */
#define Q_OVER_M_TRITON       ( 3.19964547001e7f  * (_A * _S / _KG))    /* A s / kg */

#define MACROPARTICLE_N 1e9 // Number of particles per macroparticle

enum PARTICLE_SPECIES {
    NEUTRON = 1,
    ELECTRON = 2,
    PROTON = 3,
    DEUTERIUM = 4,
    TRITIUM = 5,
    HELIUM_4_NUC = 6,
    DEUTERON = 7,
    TRITON = 8,
    ELECTRON_MACROPARTICLE = 102,
    PROTON_MACROPARTICLE = 103
};

inline float particle_mass(float species) {
    if      (species == 1.0)   return M_NEUTRON;
    else if (species == 2.0)   return M_ELECTRON;
    else if (species == 3.0)   return M_PROTON;
    else if (species == 4.0)   return M_DEUTERIUM;
    else if (species == 5.0)   return M_TRITIUM;
    else if (species == 6.0)   return M_HELIUM_4_NUC;
    else if (species == 7.0)   return M_DEUTERON;
    else if (species == 8.0)   return M_TRITON;
    else if (species == 102.0) return M_ELECTRON;
    else if (species == 103.0) return M_PROTON;

    return 0.0; // error case, should never happen
}

inline float particle_charge(float species) {
    if      (species == 1.0)   return 0;         // neutron
    else if (species == 2.0)   return -Q_E;      // electron
    else if (species == 3.0)   return Q_E;       // proton
    else if (species == 4.0)   return 0;         // deuterium
    else if (species == 5.0)   return 0;         // tritium
    else if (species == 6.0)   return 2.0 * Q_E; // helium4
    else if (species == 7.0)   return Q_E;       // ion_deuterium
    else if (species == 8.0)   return Q_E;       // ion_tritium
    else if (species == 102.0) return -Q_E * MACROPARTICLE_N;   // electron macroparticle
    else if (species == 103.0) return  Q_E * MACROPARTICLE_N;   // proton macroparticle

    return 0.0; // error case, should never happen
}

inline float charge_to_mass_ratio(float species) {
    if      (species == 1.0)   return 0.0;
    else if (species == 2.0)   return Q_OVER_M_ELECTRON;
    else if (species == 3.0)   return Q_OVER_M_PROTON;
    else if (species == 4.0)   return 0;
    else if (species == 5.0)   return 0;
    else if (species == 6.0)   return Q_OVER_M_HELIUM_4_NUC;
    else if (species == 7.0)   return Q_OVER_M_DEUTERON;
    else if (species == 8.0)   return Q_OVER_M_TRITON;
    else if (species == 102.0) return Q_OVER_M_ELECTRON * MACROPARTICLE_N;
    else if (species == 103.0) return Q_OVER_M_PROTON * MACROPARTICLE_N;

    return 0.0; // error case, should never happen
}

#endif