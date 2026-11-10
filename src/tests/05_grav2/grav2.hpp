#ifndef GRAV2_HPP
#define GRAV2_HPP

#include "grav2_common.hpp"

namespace grav2 {

enum struct HaloAlgorithm {
    NONE,
    SPHERICAL,
    NFS
};

struct HaloConfig {
    HaloAlgorithm algorithm = HaloAlgorithm::NONE;
    float R = 3.f;
    float M = 5.f;
    float nsw_concentration = 10.f;
};

struct HaloFunctor {
    virtual ~HaloFunctor() = default;
    virtual std::array<float, 2> operator()(float, float) const = 0;
};

void all_particles_method(std::vector<Body> init, float DT, size_t STEPS, int SAVE_EVERY, float SOFTENING = 1e-2f);

} // namespace grav2

#endif
