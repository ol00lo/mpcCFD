#include "grav2_common.hpp"

#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <vector>

using namespace grav2;

namespace {
std::string SAVE_DIRECTORY = ".";
}

void grav2::set_save_directory(std::string directory) {
    namespace fs = std::filesystem;
    fs::path dir = fs::path(OUTPUT_BINARY_DIR) / fs::path("output") / directory;
    if (fs::exists(dir)) {
        if (!fs::is_directory(dir))
            throw std::runtime_error("Path is not a directory: " + dir.string());
        for (const auto& entry: fs::directory_iterator(dir))
            fs::remove_all(entry.path());
    } else {
        fs::create_directories(dir);
    }

    SAVE_DIRECTORY = dir.string();
}

void grav2::save_solution_part(const std::vector<Body>& bodies, size_t step) {
    // particles
    std::string part_file_name = std::format("{}/part_{:05d}.vtk", SAVE_DIRECTORY, step);
    std::ofstream out(part_file_name);
    out << "# vtk DataFile Version 3.0\n";
    out << "Particles\n";
    out << "ASCII\n";
    out << "DATASET POLYDATA\n";
    out << "POINTS " << bodies.size() << " float\n";
    for (const auto& s: bodies) {
        out << s.x << " " << s.y << " 0\n";
    }
    out << "VERTICES " << bodies.size() << " " << (2 * bodies.size()) << "\n";
    for (size_t i = 0; i < bodies.size(); ++i) {
        out << "1 " << i << "\n";
    }
    out << "POINT_DATA " << bodies.size() << "\n";
    out << "SCALARS mass float 1\n";
    out << "LOOKUP_TABLE default\n";
    for (const auto& s: bodies)
        out << s.mass << "\n";
    out << "SCALARS velocity float 1\n";
    out << "LOOKUP_TABLE default\n";
    for (const auto& s: bodies)
        out << std::sqrt(s.vx * s.vx + s.vy * s.vy) << "\n";

    out.close();
}
