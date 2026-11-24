#include "fvm_poisson_assembler.hpp"
#include "cfd/mat/lodmat.hpp"
#include "grid/grid_partition.hpp"

using namespace cfd;

namespace {

FvmExtendedCollocations build_ecol(std::shared_ptr<IGrid> grid) {
    SubGrid* sg = dynamic_cast<SubGrid*>(grid.get());
    if (sg != nullptr) {
        const std::vector<size_t>& ff = sg->domain_connection_faces();
        std::set<size_t> interdomain_faces(ff.begin(), ff.end());
        return FvmExtendedCollocations(*grid, interdomain_faces);
    } else {
        return FvmExtendedCollocations(*grid);
    }
}

}; // namespace

FvmPoissonAssembler::FvmPoissonAssembler(std::shared_ptr<IGrid> grid, std::shared_ptr<ITestData> test_data)
    : grid_(grid),
      test_data_(test_data),
      ecol_(build_ecol(grid_)) {};

CsrMatrix FvmPoissonAssembler::assemble_lhs() const {
    LodMatrix mat(ecol_.size());

    // internal
    for (size_t iface: ecol_.all_faces()) {
        // neighbour collocation points for the given face
        auto [icolloc, jcolloc] = ecol_.tab_face_colloc(iface);
        Point xi = ecol_.points[icolloc];
        Point xj = ecol_.points[jcolloc];
        double h = vector_abs(xj - xi);
        double v = grid_->face_area(iface) / h;
        mat.add_value(icolloc, icolloc, v);
        mat.add_value(jcolloc, jcolloc, v);
        mat.add_value(icolloc, jcolloc, -v);
        mat.add_value(jcolloc, icolloc, -v);
    }
    // boundary: dirichlet
    for (size_t iface: ecol_.boundary_faces()) {
        size_t icolloc = ecol_.boundary_colloc(iface);
        mat.set_unit_row(icolloc);
    }
    return mat.to_csr();
}

std::vector<double> FvmPoissonAssembler::assemble_rhs() const {
    std::vector<double> rhs(ecol_.size(), 0.0);
    // internal
    for (size_t icolloc: ecol_.cell_collocations) {
        double value = test_data_->f(ecol_.points[icolloc]);
        size_t icell = ecol_.cell_index(icolloc);
        double volume = grid_->cell_volume(icell);
        rhs[icell] = value * volume;
    }
    // boundary: dirichlet
    for (size_t iface: ecol_.boundary_faces()) {
        // linear index of boundary collocation point
        size_t icolloc = ecol_.boundary_colloc(iface);
        rhs[icolloc] = test_data_->exact_solution(grid_->face_center(iface));
    }
    return rhs;
}