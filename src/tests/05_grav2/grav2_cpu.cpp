#include "grav2.hpp"
#include "grav2_common.hpp"
#include "tictoc.hpp"
#include <cmath>
#include <iostream>

using namespace grav2;

namespace {

void compute_part_accelerations(std::vector<Body>& bodies, float SOFTENING) {
    const float SOFTENING2 = SOFTENING * SOFTENING;

    size_t n = bodies.size();
    for (size_t i = 0; i < n; ++i) {
        bodies[i].ax = bodies[i].ay = 0;
    }

    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            float rx = 0; // TODO
            float ry = 0; // TODO
            float r2 = rx * rx + ry * ry + SOFTENING2;
            [[maybe_unused]] float r3 = r2 * std::sqrt(r2);
            // TODO: вклад частицы j в ускорение частицы i
            // TODO: вклад частицы i в ускорение частицы j
        }
    }
}

} // namespace

void grav2::all_particles_method(std::vector<Body> init, [[maybe_unused]] float DT, size_t STEPS, int SAVE_EVERY,
                                 float SOFTENING) {

    auto full_tic = dbg::ScopedTic("all_particles_method");
    std::vector<Body> bodies = std::move(init);
    [[maybe_unused]] const size_t n_parts = bodies.size();

    compute_part_accelerations(bodies, SOFTENING);

    for (size_t step = 0; step < STEPS + 1; ++step) {
        // new coordinates use acceleration
        {
            auto tic = dbg::ScopedTic("coordinates");
        }

        // new accelerations
        {
            auto tic = dbg::ScopedTic("acceleration");
        }

        // new velocities
        {
            auto tic = dbg::ScopedTic("velocity");
        }

        // save
        if (step % SAVE_EVERY == 0) {
            auto tic = dbg::ScopedTic("save");
            std::cout << "save at step = " << step << std::endl;
            save_solution_part(bodies, step);
        }
    }
}
