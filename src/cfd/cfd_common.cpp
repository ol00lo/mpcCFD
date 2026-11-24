#include "cfd_common.hpp"

#include <filesystem>
#include <fstream>
#include <print>
#include <string>
#include <system_error>
#include <utility>

namespace fs = std::filesystem;
using namespace cfd;

std::ofstream cfd::open_fstream(std::string fpath) {
    fs::path file_path(std::move(fpath));

    if (auto dir_path = file_path.parent_path(); !dir_path.empty()) {
        std::error_code ec;
        fs::create_directories(dir_path, ec);

        if (ec) {
            std::println(stderr, "Warning: Failed to create directory '{}': {}", dir_path.string(), ec.message());
            return {};
        }
    }

    std::ofstream out(file_path);
    if (!out) {
        std::println(stderr, "Warning: Failed to open file '{}'", file_path.string());
    }

    return out;
}
