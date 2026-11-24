#ifndef CFD_COMMON_HPP
#define CFD_COMMON_HPP

#include "macros.hpp"
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace cfd {

/**
 * @brief invalid index
 *
 * Used in grid connectivity tables to define blank connection
 */
[[maybe_unused]] constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);

std::ofstream open_fstream(std::string fpath);
} // namespace cfd

#endif
