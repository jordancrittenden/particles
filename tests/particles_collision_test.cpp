#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <cmath>
#include "physical_constants.h"

// Tests the physics used by particles_exact and particles_pic: two opposite charges
// (proton and electron) at rest 1 m apart should attract and collide in ~0.07 s.

namespace {

const float COLLISION_DISTANCE_M = 1e-4f;  // consider "collision" when separation < this
const float EXPECTED_COLLISION_TIME_S = 0.07f;
const float TOLERANCE_FRAC = 0.02f;  // allow ~2% variation
const float DT_S = 1e-6f;

// Coulomb acceleration of particle at pos due to other at other_pos (other has charge other_q).
// E at pos due to other: r_hat from other toward pos, E = K_E * other_charge / r^2 * r_hat.
// Returns a = (my_charge/my_mass) * E.
glm::vec3 coulomb_accel(
    glm::vec3 pos,
    glm::vec3 other_pos,
    float other_charge,
    float my_charge,
    float my_mass)
{
    glm::vec3 r = pos - other_pos;  // from source (other) to field point (pos)
    float r_mag = glm::length(r);
    if (r_mag < 1e-9f) return glm::vec3(0.f);
    float q_over_m = my_charge / my_mass;
    float E_mag = K_E * other_charge / (r_mag * r_mag);
    glm::vec3 r_hat = r / r_mag;
    return q_over_m * E_mag * r_hat;
}

void step_two_particles(
    glm::vec3& pos_e, glm::vec3& vel_e,
    glm::vec3& pos_p, glm::vec3& vel_p,
    float dt)
{
    float m_e = particle_mass(static_cast<float>(ELECTRON));
    float m_p = particle_mass(static_cast<float>(PROTON));
    float q_e = particle_charge(static_cast<float>(ELECTRON));
    float q_p = particle_charge(static_cast<float>(PROTON));

    glm::vec3 a_e = coulomb_accel(pos_e, pos_p, q_p, q_e, m_e);
    glm::vec3 a_p = coulomb_accel(pos_p, pos_e, q_e, q_p, m_p);

    vel_e += a_e * dt;
    vel_p += a_p * dt;
    pos_e += vel_e * dt;
    pos_p += vel_p * dt;
}

}  // namespace

TEST(ParticlesCollision, ProtonAndElectronAtRest1mApartCollideInAbout007s) {
    // Electron at origin, proton at (1, 0, 0); both at rest
    glm::vec3 pos_e(0.f, 0.f, 0.f);
    glm::vec3 vel_e(0.f, 0.f, 0.f);
    glm::vec3 pos_p(1.f, 0.f, 0.f);
    glm::vec3 vel_p(0.f, 0.f, 0.f);

    float t = 0.f;
    while (t < 0.2f) {  // cap to avoid infinite loop
        float d = glm::length(pos_p - pos_e);
        if (d < COLLISION_DISTANCE_M)
            break;
        step_two_particles(pos_e, vel_e, pos_p, vel_p, DT_S);
        t += DT_S;
    }

    EXPECT_LT(glm::length(pos_p - pos_e), COLLISION_DISTANCE_M)
        << "Particles did not collide within 0.2 s";
    EXPECT_GE(t, EXPECTED_COLLISION_TIME_S * (1.f - TOLERANCE_FRAC))
        << "Collision time " << t << " s is below expected range";
    EXPECT_LE(t, EXPECTED_COLLISION_TIME_S * (1.f + TOLERANCE_FRAC))
        << "Collision time " << t << " s is above expected range";
}
