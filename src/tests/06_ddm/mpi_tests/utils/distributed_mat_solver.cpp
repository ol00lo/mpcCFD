#include "distributed_mat_solver.hpp"
#include <format>

using namespace cfd;

namespace {
std::vector<double> diag_without_zero_division(const CsrMatrix& mat) {
    std::vector<double> ret = mat.diagonal();
    for (auto& v: ret) {
        if (std::abs(v) < 1e-12) {
            v = 1e-12;
        }
    }
    return ret;
}
} // namespace

DistributedSeidelMatrixSolver::DistributedSeidelMatrixSolver(CsrMatrix mat, int maxit, double eps)
    : maxit_(maxit),
      eps_(eps),
      mat_(std::move(mat)),
      diag_(diag_without_zero_division(mat_)) {}

int DistributedSeidelMatrixSolver::solve(const DistributedVector& rhs, DistributedVector& ret) const {
    if (mat_.n_rows() == 0) {
        throw std::runtime_error("Invalid matrix in SeidelMatrixSolver");
    }

    // ALGEBRAIC_SCHWARZ.2

    std::cout << std::format("WARNING: Seidel solver failed to converge in {} iterations. Final residual {}", maxit_,
                             global_residual)
              << std::endl;

    return maxit_;
}
