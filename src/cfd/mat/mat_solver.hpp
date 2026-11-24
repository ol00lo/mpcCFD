#ifndef CFD_MPC_MAT_SOLVER_HPP
#define CFD_MPC_MAT_SOLVER_HPP

#include "cfd/mat/csrmat.hpp"

namespace cfd {

class SeidelMatrixSolver {
public:
    SeidelMatrixSolver(CsrMatrix mat, int maxit = 1000, double eps = 1e-6);
    // returns number of iterations
    int solve(const std::vector<double>& rhs, std::vector<double>& ret) const;

private:
    const int maxit_;
    const double eps_;
    const CsrMatrix mat_;
    const std::vector<double> diag_;
};

} // namespace cfd

#endif
