#include "mat_solver.hpp"
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

SeidelMatrixSolver::SeidelMatrixSolver(CsrMatrix mat, int maxit, double eps)
    : maxit_(maxit),
      eps_(eps),
      mat_(std::move(mat)),
      diag_(diag_without_zero_division(mat_)) {}

int SeidelMatrixSolver::solve(const std::vector<double>& rhs, std::vector<double>& ret) const {
    if (mat_.n_rows() == 0) {
        throw std::runtime_error("Invalid matrix in SeidelMatrixSolver");
    }
    ret.resize(mat_.n_rows(), 0.0);

    double residual;
    for (int it = 0; it < maxit_; ++it) {
        residual = 0;
        for (size_t irow = 0; irow < mat_.n_rows(); ++irow) {
            double res_row = rhs[irow] - mat_.mult_vec(irow, ret);
            ret[irow] += res_row / diag_[irow];
            residual = std::max(residual, std::abs(res_row));
        }

        if (residual < eps_) {
            return it;
        }
    }
    std::cout << std::format("WARNING: Seidel solver failed to converge in {} iterations. Final residual {}", maxit_,
                             residual)
              << std::endl;
    ;
    return maxit_;
}