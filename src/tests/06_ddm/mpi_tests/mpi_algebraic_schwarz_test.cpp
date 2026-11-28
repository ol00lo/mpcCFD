#include "cfd/fvm_poisson_assembler.hpp"
#include "cfd/grid/grid_partition.hpp"
#include "cfd/grid/unstructured_grid2d.hpp"
#include "cfd/grid/vtk.hpp"
#include "cfd/mat/mat_solver.hpp"
#include "mpi_test_common.hpp"
#include "utils/distributed_mat_solver.hpp"
#include "utils/distributed_vector.hpp"

using namespace cfd;

namespace {

constexpr double SOLVER_TOL = 1e-6;
constexpr size_t SOLVER_MAXIT = 1'000;

// double compute_residual(const CsrMatrix& m, const std::vector<double>& rhs, const std::vector<double>& u) {
//     double ret = 0;
//     std::vector<double> mvec = m.mult_vec(u);
//     for (size_t i = 0; i < m.n_rows(); ++i) {
//         ret = std::max(ret, std::abs(mvec[i] - rhs[i]));
//     }
//     return ret;
// }

struct TestData2D : public FvmPoissonAssembler::ITestData {
    double exact_solution(Point p) const override {
        double x = p.x;
        double y = p.y;
        return std::sin(2 * M_PI * x + 3) + std::cos(10 * y * y);
    }
    double f(Point p) const override {
        double x = p.x;
        double y = p.y;
        return 20.0 * std::sin(10 * y * y) + 400 * y * y * std::cos(10 * y * y) +
               4 * M_PI * M_PI * std::sin(2 * M_PI * x + 3);
    }
};

std::array<int, 2> get_partition(size_t mpi_size) {
    switch (mpi_size) {
    case 1:
        return {1, 1};
    case 2:
        return {1, 2};
    case 3:
        return {1, 3};
    case 4:
        return {2, 2};
    case 6:
        return {2, 3};
    case 8:
        return {2, 4};
    default:
        throw std::runtime_error("Invalid mpi size. Please choose any of [1, 2, 3, 4, 6, 8]");
    }
}

} // namespace

TEST_CASE("Algebraic Schwarz", "[algebraic-schwarz]") {
    constexpr size_t NBUFFER = 0;
    int mpi_rank, mpi_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    int idom = mpi_rank;

    auto test_data = std::make_shared<TestData2D>();
    // read grid
    std::string grid_fn = "/app/src/tests/06_ddm/test_data/trigrid_500.vtk";
    auto grid = UnstructuredGrid2D::vtk_read_p(grid_fn);
    [[maybe_unused]] size_t n_colloc = grid->n_cells() + grid->n_boundary_faces();

    // compute full solution: on master rank only
    std::vector<double> full_u;
    if (mpi_rank == 0) {
        FvmPoissonAssembler wrk(grid, test_data);
        auto mat = wrk.assemble_lhs();
        auto rhs = wrk.assemble_rhs();
        SeidelMatrixSolver slv(std::move(mat), SOLVER_MAXIT, SOLVER_TOL);
        slv.solve(rhs, full_u);
    }

    // grid parition
    GridPartition gridpart = GridPartition::build_uniform<2>(grid, get_partition(mpi_size), NBUFFER);
    std::shared_ptr<SubGrid> local_grid = gridpart.subgrid(idom);

    // assemble subproblems
    FvmPoissonAssembler wrk(local_grid, test_data);
    CsrMatrix local_mat = wrk.assemble_lhs();
}
