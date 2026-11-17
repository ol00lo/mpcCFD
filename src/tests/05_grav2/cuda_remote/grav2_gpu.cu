#include "grav2.hpp"
#include "grav2_common.hpp"
#include "tictoc.hpp"
#include <thrust/device_vector.h>

using namespace grav2;

namespace {
constexpr int N_THREADS = 128;

__global__ void check_kernel(int n, Body* bodies) {
    int tid = threadIdx.x;
    int bid = blockIdx.x;
    int ibody = bid * blockDim.x + tid;

    if (ibody >= n) {
        return;
    }

    if (ibody == 0) {
        printf("%d bodies received in cuda kernel\n", n);
        printf("bodies[0].x = %f\n", bodies[ibody].x);
    }
}

__global__ void compute_part_accelerations(int n, Body* bodies, float SOFTENING) {
    int tid = threadIdx.x;
    int bid = blockIdx.x;
    int i = bid * blockDim.x + tid;
    if (i >= n) {
        return;
    }
    const float SOFTENING2 = SOFTENING * SOFTENING;

    bodies[i].ax = bodies[i].ay = 0;

    for (size_t j = 0; j < n; ++j) {
        // TODO: compute accelerations
    }
}

__global__ void compute_coordinates(int n, Body* bodies, const Body* bodies_old, float DT) {
    // TODO: compute coordinates
}

__global__ void compute_velocity(int n, Body* bodies, const Body* bodies_old, float DT) {
    // TODO: compute velocity
}

} // namespace

void grav2::all_particles_method(std::vector<Body> init, [[maybe_unused]] float DT, size_t STEPS, int SAVE_EVERY,
                                 float SOFTENING) {

    auto full_tic = dbg::ScopedTic("all_particles_method");
    thrust::device_vector<Body> bodies_dev(init.begin(), init.end());
    int n = static_cast<int>(bodies_dev.size());

    Body* bodies_ptr = bodies_dev.data().get();

    int n_blocks = (n + N_THREADS - 1) / N_THREADS;
    check_kernel<<<n_blocks, N_THREADS>>>(n, bodies_ptr);
    cudaDeviceSynchronize();

    compute_part_accelerations<<<n_blocks, N_THREADS>>>(n, bodies_ptr, SOFTENING);

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
            // TODO: copy

            std::cout << "save at step = " << step << std::endl;
            save_solution_part(init, step);
        }
    }
}
