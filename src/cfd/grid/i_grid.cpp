#include "i_grid.hpp"

using namespace cfd;

///////////////////////////////////////////////////////////////////////////////
// Cache
///////////////////////////////////////////////////////////////////////////////
void IGrid::Cache::clear() {
    boundary_faces.clear();
    boundary_points.clear();
    boundary_cells.clear();
    point_cell.clear();
}

void IGrid::Cache::need_boundary_faces(const IGrid& grid) {
    if (boundary_faces.size() > 0) {
        return;
    }
    for (size_t iface = 0; iface < grid.n_faces(); ++iface) {
        std::array<size_t, 2> cc = grid.tab_face_cell(iface);
        if (cc[0] == INVALID_INDEX || cc[1] == INVALID_INDEX) {
            boundary_faces.push_back(iface);
        }
    }
}

void IGrid::Cache::need_boundary_points(const IGrid& grid) {
    if (boundary_points.size() > 0) {
        return;
    }
    need_boundary_faces(grid);
    std::set<size_t> points;
    for (size_t iface: boundary_faces) {
        for (size_t ipoint: grid.tab_face_point(iface)) {
            points.insert(ipoint);
        }
    }
    boundary_points = std::vector<size_t>(points.begin(), points.end());
}

void IGrid::Cache::need_boundary_cells(const IGrid& grid) {
    if (boundary_cells.size() > 0) {
        return;
    }
    need_boundary_faces(grid);
    std::set<size_t> cells;
    for (size_t iface: boundary_faces) {
        auto [c1, c2] = grid.tab_face_cell(iface);
        if (c1 == INVALID_INDEX) {
            c1 = c2;
        }
        cells.insert(c1);
    }
    boundary_cells = std::vector<size_t>(cells.begin(), cells.end());
}

void IGrid::Cache::need_internal_faces(const IGrid& grid) {
    if (internal_faces.size() > 0) {
        return;
    }
    for (size_t iface = 0; iface < grid.n_faces(); ++iface) {
        std::array<size_t, 2> cc = grid.tab_face_cell(iface);
        if (cc[0] != INVALID_INDEX && cc[1] != INVALID_INDEX) {
            internal_faces.push_back(iface);
        }
    }
}

void IGrid::Cache::need_point_cell(const IGrid& grid) {
    if (point_cell.size() > 0) {
        return;
    }
    point_cell.resize(grid.n_points());
    for (size_t icell = 0; icell < grid.n_cells(); ++icell) {
        for (size_t ipoint: grid.tab_cell_point(icell)) {
            point_cell[ipoint].push_back(icell);
        }
    }
    for (auto& pc: point_cell) {
        std::sort(pc.begin(), pc.end());
    }
}

void IGrid::Cache::need_face_bnd(const IGrid& grid) {
    if (face_bnd.size() > 0) {
        return;
    }
    need_boundary_faces(grid);
    face_bnd.resize(grid.n_faces(), INVALID_INDEX);
    for (size_t ibnd = 0; ibnd < boundary_faces.size(); ++ibnd) {
        face_bnd[boundary_faces[ibnd]] = ibnd;
    }
}

void IGrid::Cache::need_boundary_face_info(const IGrid& grid) {
    if (boundary_face_info.size() > 0) {
        return;
    }
    need_boundary_faces(grid);
    for (size_t ibnd = 0; ibnd < boundary_faces.size(); ++ibnd) {
        size_t iface = boundary_faces[ibnd];
        auto [icell, cell_right] = grid.tab_face_cell(iface);
        Vector outer_normal = grid.face_normal(iface);
        bool is_reverted = false;
        if (icell == INVALID_INDEX) {
            icell = cell_right;
            outer_normal *= -1.0;
            is_reverted = true;
        }
        boundary_face_info.push_back(IGrid::BoundaryFaceInfo{
            .iface = iface, .ibnd = ibnd, .icell = icell, .outer_normal = outer_normal, .is_reverted = is_reverted});
    }
}

///////////////////////////////////////////////////////////////////////////////
// IGrid
///////////////////////////////////////////////////////////////////////////////
std::vector<size_t> IGrid::boundary_faces() const {
    cache_.need_boundary_faces(*this);
    return cache_.boundary_faces;
}

size_t IGrid::n_boundary_faces() const {
    cache_.need_boundary_faces(*this);
    return cache_.boundary_faces.size();
}

std::vector<size_t> IGrid::boundary_points() const {
    cache_.need_boundary_points(*this);
    return cache_.boundary_points;
}

std::vector<size_t> IGrid::boundary_cells() const {
    cache_.need_boundary_cells(*this);
    return cache_.boundary_cells;
}

std::vector<size_t> IGrid::internal_faces() const {
    cache_.need_internal_faces(*this);
    return cache_.internal_faces;
}

std::vector<size_t> IGrid::tab_point_cell(size_t ipoint) const {
    cache_.need_point_cell(*this);
    return cache_.point_cell[ipoint];
}

std::pair<Point, Point> IGrid::box() const {
    Point pmin = point(0);
    Point pmax = point(0);

    for (size_t i: boundary_points()) {
        Point pi = point(i);
        pmin.x = std::min(pmin.x, pi.x);
        pmin.y = std::min(pmin.y, pi.y);
        pmin.z = std::min(pmin.z, pi.z);

        pmax.x = std::max(pmax.x, pi.x);
        pmax.y = std::max(pmax.y, pi.y);
        pmax.z = std::max(pmax.z, pi.z);
    }

    return {pmin, pmax};
}

size_t IGrid::face_boundary_index(size_t iface) const {
    cache_.need_face_bnd(*this);
    return cache_.face_bnd[iface];
}

const IGrid::BoundaryFaceInfo& IGrid::boundary_face_info(size_t iface) const {
    cache_.need_boundary_face_info(*this);
    size_t ibnd = face_boundary_index(iface);
    if (ibnd == INVALID_INDEX) {
        throw std::runtime_error(std::format("Face {} is not a boundary face", iface));
    }
    return cache_.boundary_face_info[ibnd];
}