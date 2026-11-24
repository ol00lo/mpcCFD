#include "catch2/catch_all.hpp"
#include "cfd/fvm_poisson_assembler.hpp"
#include "cfd/grid/grid_partition.hpp"
#include "cfd/grid/unstructured_grid2d.hpp"
#include "cfd/grid/vtk.hpp"
#include "cfd/mat/mat_solver.hpp"

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

} // namespace

TEST_CASE("Shwarz multiplicative", "[schwarz-multiplicative]") {
    constexpr int NBUFFER = 2;
    auto test_data = std::make_shared<TestData2D>();
    // read grid
    std::string grid_fn = "/app/src/tests/06_ddm/test_data/trigrid_500.vtk";
    auto grid = UnstructuredGrid2D::vtk_read_p(grid_fn);
    size_t n_colloc = grid->n_cells() + grid->n_boundary_faces();

    // compute full solution
    std::vector<double> full_u(n_colloc, 0);
    {
        FvmPoissonAssembler wrk(grid, test_data);
        auto mat = wrk.assemble_lhs();
        auto rhs = wrk.assemble_rhs();
        SeidelMatrixSolver slv(std::move(mat), SOLVER_MAXIT, SOLVER_TOL);
        slv.solve(rhs, full_u);
    }

    // grid parition
    std::vector<double> global_u(n_colloc, 0);
    std::vector<double> error(n_colloc, 0);
    GridPartition gridpart = GridPartition::build_uniform<2>(grid, {1, 2}, NBUFFER);
    std::vector<std::shared_ptr<SubGrid>> local_grid;
    for (size_t idom = 0; idom < gridpart.n_domains(); ++idom) {
        local_grid.push_back(gridpart.subgrid(idom));
    }

    // blending functions
    std::vector<std::vector<double>> local_Xi;
    for (size_t idom = 0; idom < gridpart.n_domains(); ++idom) {
        local_Xi.push_back(gridpart.extended_blending_weights(idom, GridPartition::Restriction::AVERAGE));
    }

    // assemble subproblems
    std::vector<CsrMatrix> local_mat;
    std::vector<std::vector<double>> local_rhs;
    std::vector<std::vector<double>> local_u;
    std::vector<std::shared_ptr<SeidelMatrixSolver>> local_solver;
    for (size_t idom = 0; idom < gridpart.n_domains(); ++idom) {
        FvmPoissonAssembler wrk(local_grid[idom], test_data);
        local_mat.push_back(wrk.assemble_lhs());
        // interdomain-bc
        for (size_t local_ghost: local_grid[idom]->ghost_cells()) {
            local_mat[idom].set_unit_row(local_ghost);
        }
        local_rhs.push_back(wrk.assemble_rhs());
        local_u.emplace_back(local_rhs.back().size(), 0);
        local_solver.emplace_back(new SeidelMatrixSolver(local_mat.back(), SOLVER_MAXIT, SOLVER_TOL));
    }

    // solution vtk writer initialization
    VtkUtils::TimeSeriesWriter writer("/app/build/bin/output/poisson_ddm");

    // Schwarz iterations
    size_t it{};
    double global_residual{};
    for (it = 0; it < 1'000; ++it) {
        global_residual = 0;

        // multiplicative method
        for (size_t idom = 0; idom < gridpart.n_domains(); ++idom) {
            // interdomain-bc
            for (size_t local_ghost: local_grid[idom]->ghost_cells()) {
                size_t global_ghost = local_grid[idom]->to_global_cell(local_ghost);
                local_rhs[idom][local_ghost] = global_u[global_ghost];
            }
            // --- REDUCTION 1
            // residual
            double local_residual = compute_residual(local_mat[idom], local_rhs[idom], local_u[idom]);
            global_residual = std::max(global_residual, local_residual);

            // local solution
            auto local_u_old = local_u[idom];
            local_solver[idom]->solve(local_rhs[idom], local_u[idom]);

            // --- REDUCTION 2
            // build global solution = old + Xi * (u_new - uold)
            std::vector<double> xi_du(local_u[idom].size());
            for (size_t i = 0; i < xi_du.size(); ++i) {
                xi_du[i] = (local_u[idom][i] - local_u_old[i]) * local_Xi[idom][i];
            }
            std::vector<double> local_u_expanded = local_grid[idom]->expand_extended_vector(xi_du);
            for (size_t i = 0; i < global_u.size(); ++i) {
                global_u[i] += local_u_expanded[i];
            }
        }
        // info
        std::cout << "it=" << it << ", residual=" << global_residual << std::endl;

        // save solution
        std::string out_filename = writer.add(static_cast<double>(it));
        grid->save_vtk(out_filename);
        VtkUtils::add_cell_data(global_u, "solution", out_filename, grid->n_cells());
        for (size_t i = 0; i < grid->n_cells(); ++i)
            error[i] = std::abs(full_u[i] - global_u[i]);
        VtkUtils::add_cell_data(error, "error", out_filename, grid->n_cells());

        // exit condition
        if (global_residual < 1e-6) {
            break;
        }
    }

    CHECK(it == 8);
    CHECK(global_residual == Catch::Approx(9.65254e-07).margin(1e-9));
}

TEST_CASE("Schwarz-additive", "[schwarz-additive]") {}
