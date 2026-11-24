#include "densemat.hpp"
#include "cfd/mat/csrmat.hpp"

using namespace cfd;

DenseMatrix::DenseMatrix(size_t nrows, size_t ncols) : vals(nrows * ncols, 0), nrows_(nrows), ncols_(ncols) {}

DenseMatrix::DenseMatrix(size_t nrows, size_t ncols, const std::vector<double>& values)
    : vals(values),
      nrows_(nrows),
      ncols_(ncols) {}

void DenseMatrix::set_value(size_t irow, size_t icol, double value) {
    vals[irow * ncols_ + icol] = value;
}

DenseMatrix DenseMatrix::transpose() const {
    DenseMatrix ret(ncols_, nrows_);

    for (size_t i = 0; i < nrows_; ++i)
        for (size_t j = 0; j < ncols_; ++j) {
            size_t old_index = i * ncols_ + j;
            size_t new_index = j * nrows_ + i;

            ret.vals[new_index] = vals[old_index];
        }

    return ret;
}

DenseMatrix DenseMatrix::mult_mat(const DenseMatrix& mat) const {
    if (n_cols() != mat.n_rows()) {
        _THROW_INTERNAL_ERROR_;
    }
    DenseMatrix ret(n_rows(), mat.n_cols());
    for (size_t i = 0; i < n_rows(); ++i)
        for (size_t j = 0; j < mat.n_cols(); ++j) {
            double sum = 0;
            for (size_t k = 0; k < n_cols(); ++k) {
                sum += value(i, k) * mat.value(k, j);
            }
            ret.set_value(i, j, sum);
        }

    return ret;
}

DenseMatrix DenseMatrix::inverse() const {
    if (nrows_ == 1) {
        return DenseMatrix(1, 1, {1.0 / vals[0]});
    } else if (nrows_ == 2) {
        double det = vals[0] * vals[3] - vals[1] * vals[2];
        return DenseMatrix(2, 2, {vals[3] / det, -vals[2] / det, -vals[1] / det, vals[0] / det});
    } else if (nrows_ == 3) {
        _THROW_NOT_IMP_;
    } else {
        _THROW_NOT_IMP_;
    }
}

size_t DenseMatrix::n_cols() const {
    return ncols_;
}

size_t DenseMatrix::n_rows() const {
    return nrows_;
}

double DenseMatrix::value(size_t irow, size_t icol) const {
    return vals[irow * ncols_ + icol];
}

std::vector<double> DenseMatrix::mult_vec_p(const double*) const {
    _THROW_NOT_IMP_;
}

double DenseMatrix::mult_vec_p(size_t, const double*) const {
    _THROW_NOT_IMP_;
}

CsrMatrix DenseMatrix::to_csr() const {
    std::vector<size_t> addr1{0};
    std::vector<size_t> cols1;
    std::vector<double> vals1;

    auto it = vals.begin();
    for (size_t i = 0; i < n_rows(); ++i) {
        addr1.push_back(addr1.back());
        for (size_t j = 0; j < n_cols(); ++j) {
            if (std::abs(*it) > 1e-16) {
                addr1.back() += 1;
                cols1.push_back(j);
                vals1.push_back(*it);
            }
            ++it;
        }
    }

    return CsrMatrix(std::move(addr1), std::move(cols1), std::move(vals1));
}