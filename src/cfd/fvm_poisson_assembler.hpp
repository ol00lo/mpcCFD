#ifndef MPC_CFD_FVM_POISSON_ASSEMBLER_HPP
#define MPC_CFD_FVM_POISSON_ASSEMBLER_HPP

#include "cfd/fvm/fvm_extended_collocations.hpp"
#include "cfd/grid/i_grid.hpp"
#include "cfd/mat/csrmat.hpp"

namespace cfd {

class FvmPoissonAssembler {
public:
    struct ITestData {
        virtual ~ITestData() = default;
        virtual double exact_solution(Point p) const = 0;
        virtual double f(Point p) const = 0;
    };

    FvmPoissonAssembler(std::shared_ptr<IGrid> grid, std::shared_ptr<ITestData> test_data);

    CsrMatrix assemble_lhs() const;
    std::vector<double> assemble_rhs() const;

    size_t n_colloc() const {
        return ecol_.size();
    }

private:
    std::shared_ptr<IGrid> grid_;
    std::shared_ptr<ITestData> test_data_;
    const FvmExtendedCollocations ecol_;
};

} // namespace cfd

#endif
