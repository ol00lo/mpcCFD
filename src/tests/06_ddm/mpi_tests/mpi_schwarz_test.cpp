#include "cfd/fvm_poisson_assembler.hpp"
#include "cfd/grid/grid_partition.hpp"
#include "cfd/grid/unstructured_grid2d.hpp"
#include "cfd/grid/vtk.hpp"
#include "cfd/mat/mat_solver.hpp"
#include "mpi_test_common.hpp"

using namespace cfd;

namespace {

constexpr double SOLVER_TOL = 1e-6;
constexpr size_t SOLVER_MAXIT = 10'000;

double compute_residual(const CsrMatrix& m, const std::vector<double>& rhs, const std::vector<double>& u) {
    double ret = 0;
    std::vector<double> mvec = m.mult_vec(u);
    for (size_t i = 0; i < m.n_rows(); ++i) {
        ret = std::max(ret, std::abs(mvec[i] - rhs[i]));
    }
    return ret;
}

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

TEST_CASE("Schwarz", "[schwarz]") {
    const size_t NBUFFER = 2;
    int mpi_rank, mpi_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    int idom = mpi_rank;

    auto test_data = std::make_shared<TestData2D>();
    // read grid
    std::string grid_fn = "/app/src/tests/06_ddm/test_data/trigrid_500.vtk";
    auto grid = UnstructuredGrid2D::vtk_read_p(grid_fn);
    size_t n_colloc = grid->n_cells() + grid->n_boundary_faces();

    // compute full solution: on master rank only
    std::vector<double> full_u(n_colloc, 0);
    if (mpi_rank == 0) {
        FvmPoissonAssembler wrk(grid, test_data);
        auto mat = wrk.assemble_lhs();
        auto rhs = wrk.assemble_rhs();
        SeidelMatrixSolver slv(std::move(mat), SOLVER_MAXIT, SOLVER_TOL);
        slv.solve(rhs, full_u);
    }

    // grid parition
    std::vector<double> global_u(n_colloc, 0);
    std::vector<double> error(n_colloc, 0);
    GridPartition gridpart = GridPartition::build_uniform<2>(grid, get_partition(mpi_size), NBUFFER);
    std::shared_ptr<SubGrid> local_grid = gridpart.subgrid(idom);

    // restriction functions
    std::vector<double> local_Xi = gridpart.extended_blending_weights(idom, GridPartition::Restriction::AVERAGE);

    // assemble subproblems
    FvmPoissonAssembler wrk(local_grid, test_data);
    CsrMatrix local_mat = wrk.assemble_lhs();
    // interdomain-bc
    for (size_t local_ghost: local_grid->ghost_cells()) {
        local_mat.set_unit_row(local_ghost);
    }
    std::vector<double> local_rhs = wrk.assemble_rhs();
    std::vector<double> local_u(local_rhs.size(), 0);
    auto local_solver = std::make_shared<SeidelMatrixSolver>(local_mat, SOLVER_MAXIT, SOLVER_TOL);

    // solution vtk writer initialization
    std::shared_ptr<VtkUtils::TimeSeriesWriter> writer;
    if (mpi_rank == 0) {
        writer = std::make_shared<VtkUtils::TimeSeriesWriter>("/app/build/bin/output/poisson_ddm");
    }

    // Schwarz iterations
    size_t it;
    double global_residual;
    double local_residual;
    for (it = 0; it < 1'000; ++it) {
        // build local solution
        {
            // interdomain-bc from current global solution
            for (size_t local_ghost: local_grid->ghost_cells()) {
                size_t global_ghost = local_grid->to_global_cell(local_ghost);
                local_rhs[local_ghost] = global_u[global_ghost];
            }
            // residual
            local_residual = compute_residual(local_mat, local_rhs, local_u);

            // local solution
            local_solver->solve(local_rhs, local_u);
        }

        // --- REDUCTION 1.
        // reduce residual
        MPI_Allreduce(&local_residual, &global_residual, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

        // --- REDUCTION 2.
        // global solution
        std::fill(global_u.begin(), global_u.end(), 0.0);
        {
            std::vector<double> xi_u(local_u.size());
            for (size_t i = 0; i < xi_u.size(); ++i) {
                xi_u[i] = local_u[i] * local_Xi[i];
            }
            std::vector<double> local_u_expanded = local_grid->expand_extended_vector(xi_u);
            MPI_Allreduce(local_u_expanded.data(), global_u.data(), static_cast<int>(global_u.size()), MPI_DOUBLE,
                          MPI_SUM, MPI_COMM_WORLD);
        }

        // info
        if (mpi_rank == 0) {
            std::cout << "it=" << it << ", residual=" << global_residual << std::endl;

            // save solution
            std::string out_filename = writer->add_iter(it);
            grid->save_vtk(out_filename);
            VtkUtils::add_cell_data(global_u, "solution", out_filename, grid->n_cells());
            for (size_t i = 0; i < grid->n_cells(); ++i)
                error[i] = std::abs(full_u[i] - global_u[i]);
            VtkUtils::add_cell_data(error, "error", out_filename, grid->n_cells());
        }

        // exit condition
        if (global_residual < 1e-6) {
            break;
        }
    }

    if (mpi_rank == 0) {
        if (mpi_size == 1) {
            CHECK(it == 1);
            CHECK(global_residual == Catch::Approx(9.28733e-07).margin(1e-9));
        } else if (mpi_size == 2) {
            CHECK(it == 14);
            CHECK(global_residual == Catch::Approx(6.916e-7).margin(1e-9));
        } else if (mpi_size == 3) {
            CHECK(it == 15);
            CHECK(global_residual == Catch::Approx(8.2928e-07).margin(1e-9));
        } else if (mpi_size == 4) {
            CHECK(it == 25);
            CHECK(global_residual == Catch::Approx(8.47139e-07).margin(1e-9));
        } else if (mpi_size == 6) {
            CHECK(it == 22);
            CHECK(global_residual == Catch::Approx(9.90364e-07).margin(1e-9));
        } else if (mpi_size == 8) {
            CHECK(it == 38);
            CHECK(global_residual == Catch::Approx(9.06426e-07).margin(1e-9));
        }
    }
}