#ifndef GRAV2_COMMON_HPP
#define GRAV2_COMMON_HPP

#include <string>
#include <vector>

namespace grav2 {

[[maybe_unused]] constexpr float G = 1.0;

struct Body {
    float x, y;
    float vx, vy;
    float ax, ay;
    float mass;
};

void set_save_directory(std::string directory);
void save_solution_part(const std::vector<Body>& bodies, size_t step);

} // namespace grav2

#endif
