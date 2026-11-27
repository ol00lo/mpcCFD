#ifndef CFD_MPC_DISTRIBUTED_MAT_SOLVER_HPP
#define CFD_MPC_DISTRIBUTED_MAT_SOLVER_HPP

#include "cfd/mat/csrmat.hpp"
#include "distributed_vector.hpp"

namespace cfd {

class DistributedSeidelMatrixSolver {
public:
    DistributedSeidelMatrixSolver(CsrMatrix mat, int maxit = 1000, double eps = 1e-6);
    int solve(const DistributedVector& rhs, DistributedVector& ret) const;

private:
    const int maxit_;
    const double eps_;
    const CsrMatrix mat_;
    const std::vector<double> diag_;
};

} // namespace cfd

#endif
