#include <catch2/catch_all.hpp>

#include "tictoc.hpp"
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#include "grav2.hpp"
#include "grav2_common.hpp"

using namespace grav2;

namespace {

std::vector<Body> init_data(size_t n_bodies, unsigned seed = 42) {
    float rmax = 1.f;
    float rmin = 0.001f * rmax;
    std::mt19937 gen(seed);
    std::uniform_real_distribution<float> RAD(rmin, rmax);
    std::uniform_real_distribution<float> PHI(0, static_cast<float>(2 * std::numbers::pi));
    std::uniform_real_distribution<float> MASS(0.01f, 1.f);

    std::vector<Body> ret;

    // mass
    float total_mass = 0;
    for (size_t i = 0; i < n_bodies; ++i) {
        Body s{};
        s.mass = MASS(gen);
        s.ax = s.ay = 0.0;
        s.vx = s.vy = 0.0;
        s.x = s.y = 0.0;
        total_mass += s.mass;
        ret.push_back(s);
    }
    for (auto& s: ret) {
        s.mass /= total_mass;
    }
    total_mass = 1.f;

    // coords
    for (auto& s: ret) {
        float r = RAD(gen);
        float phi = PHI(gen);

        s.x = r * std::cos(phi);
        s.y = r * std::sin(phi);

        // approx velocity by radius homogenious mass distribution
        float m_inside = total_mass * (r * r) / (rmax * rmax);
        float v = 2 * std::sqrt(G * m_inside / r);

        s.vx = -v * std::sin(phi);
        s.vy = v * std::cos(phi);
    }

    return ret;
}

} // namespace

TEST_CASE("Gravity particle", "[grav-two-part]") {
    dbg::SetTicTimeUnit<std::chrono::microseconds>();
    constexpr float DT = 0.01f;
    constexpr int STEPS = 5000;
    constexpr float MASS = 1.f;
    constexpr float D = 2.f;
    constexpr float VEL = std::sqrt(G * MASS / (2 * D));
    constexpr int SAVE_EVERY = 100;

    Body body1;
    body1.x = -D / 2;
    body1.y = 0;
    body1.mass = MASS;
    body1.vx = 0;
    body1.vy = -VEL;
    Body body2;
    body2.x = D / 2;
    body2.y = 0;
    body2.mass = MASS;
    body2.vx = 0;
    body2.vy = VEL;
    set_save_directory("grav-two-part");
    all_particles_method({body1, body2}, DT, STEPS, SAVE_EVERY);
}

TEST_CASE("Gravity particle", "[grav-multi-part]") {
    dbg::SetTicTimeUnit<std::chrono::microseconds>();
    constexpr size_t NSTARS = 1'000;
    constexpr float DT = 0.001f;
    constexpr int STEPS = 10'000;
    constexpr int SAVE_EVERY = 20;

    std::vector<Body> bodies = init_data(NSTARS);
    set_save_directory("grav-multi-part");
    all_particles_method(std::move(bodies), DT, STEPS, SAVE_EVERY, 1e-2f);
}