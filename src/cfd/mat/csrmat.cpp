#include "csrmat.hpp"
#include "cfd/mat/densemat.hpp"
#include "cfd/mat/matrix_iter.hpp"
#include "lodmat.hpp"
#include <numeric>

using namespace cfd;

void CsrStencil::set_stencil(std::vector<size_t>&& addr_vec, std::vector<size_t>&& cols_vec) {
    addr = std::move(addr_vec);
    cols = std::move(cols_vec);
}

void CsrStencil::set_stencil(const std::vector<std::set<size_t>>& stencil_set) {
    addr = std::vector<size_t>(1, 0);
    cols.clear();

    for (size_t irow = 0; irow < stencil_set.size(); ++irow) {
        const std::set<size_t>& cols_set = stencil_set[irow];
        addr.push_back(addr.back() + cols_set.size());
        for (size_t col: cols_set) {
            cols.push_back(col);
        }
    }
}

void CsrStencil::set_stencil(const CsrStencil& stencil) {
    auto a = stencil.addr;
    auto c = stencil.cols;
    set_stencil(std::move(a), std::move(c));
}

size_t CsrStencil::n_nonzeros() const {
    return cols.size();
}

size_t CsrStencil::n_rows() const {
    return addr.size() - 1;
}

size_t CsrStencil::n_cols() const {
    return *max_element(cols.begin(), cols.end()) + 1;
}

void CsrStencil::validate() const {
    // sizes
    if (addr.size() < 1) {
        throw std::runtime_error("addr array should have more then zero entries");
    }
    if (cols.size() != addr.back()) {
        throw std::runtime_error("cols array size should match last addr entry");
    }
    // non-decreasing
    for (size_t i = 1; i < addr.size(); ++i) {
        if (addr[i] < addr[i - 1]) {
            throw std::runtime_error("addr array should be non-decreasing");
        }
    }
}

bool CsrStencil::is_in_stencil(size_t irow, size_t icol) const {
    size_t start = addr.at(irow);
    size_t end = addr.at(irow + 1);
    for (size_t i = start; i < end; ++i) {
        if (cols.at(i) == icol) {
            return true;
        }
    }
    return false;
}

double CsrStencil::value(size_t, size_t) const {
    throw std::runtime_error("CsrStencil has no values");
}

std::vector<double> CsrStencil::mult_vec_p(const double*) const {
    throw std::runtime_error("CsrStencil has no values");
}

double CsrStencil::mult_vec_p(size_t, const double*) const {
    throw std::runtime_error("CsrStencil has no values");
}

size_t CsrStencil::get_address(size_t irow, size_t icol) const {
    std::vector<size_t>::const_iterator it_start = cols.begin() + addr.at(irow);
    std::vector<size_t>::const_iterator it_end = cols.begin() + addr.at(irow + 1);
    auto fnd = std::lower_bound(it_start, it_end, icol);
    if (fnd != it_end && *fnd == icol) {
        size_t a = fnd - cols.begin();
        return a;
    }
    return INVALID_INDEX;
}

void CsrMatrix::set_values(std::vector<double>&& vals_vec) {
    vals = std::move(vals_vec);
}

void CsrMatrix::validate() const {
    CsrStencil::validate();

    // values size
    if (vals.size() != n_nonzeros()) {
        throw std::runtime_error("values array should have same size as the columns arrays");
    }
}

double CsrMatrix::value(size_t irow, size_t icol) const {
    size_t a = get_address(irow, icol);
    if (a != INVALID_INDEX) {
        return vals[a];
    } else {
        return 0.0;
    }
}

std::vector<double> CsrMatrix::mult_vec_p(const double* u) const {
    const std::vector<size_t>& a = addr;
    const std::vector<size_t>& c = cols;
    const std::vector<double>& v = vals;

    std::vector<double> ret(n_rows(), 0);
    for (size_t irow = 0; irow < n_rows(); ++irow) {
        size_t start = a[irow];
        size_t end = a[irow + 1];
        for (size_t i = start; i < end; ++i) {
            ret[irow] += v[i] * u[c[i]];
        }
    }

    return ret;
}

double CsrMatrix::mult_vec_p(size_t irow, const double* u) const {
    const std::vector<size_t>& a = addr;
    const std::vector<size_t>& c = cols;
    const std::vector<double>& v = vals;

    double ret = 0;
    size_t start = a.at(irow);
    size_t end = a.at(irow + 1);
    for (size_t i = start; i < end; ++i) {
        ret += v[i] * u[c[i]];
    }
    return ret;
}

void CsrMatrix::set_unit_row(size_t irow) {
    const std::vector<size_t>& a = addr;
    const std::vector<size_t>& c = cols;

    const size_t start = a.at(irow);
    const size_t end = a.at(irow + 1);
    for (size_t i = start; i < end; ++i) {
        vals[i] = (c[i] == irow) ? 1.0 : 0.0;
    }
}

void CsrMatrix::set_diagonal(const std::vector<double>& diag_vals) {
    const std::vector<size_t>& a = addr;
    const std::vector<size_t>& c = cols;

    for (size_t irow = 0; irow < n_rows(); ++irow) {
        const size_t start = a.at(irow);
        const size_t end = a.at(irow + 1);
        for (size_t i = start; i < end; ++i) {
            if (c[i] == irow) {
                vals[i] = diag_vals[irow];
            }
        }
    }
}

CsrMatrix cfd::assemble_block_matrix(size_t block_n_rows, size_t block_n_cols,
                                     const std::vector<std::vector<const CsrMatrix*>>& blocks) {

    size_t nrows = blocks.size() * block_n_rows;
    std::vector<size_t> cols;
    std::vector<double> vals;
    std::vector<size_t> n_row_nonzeros(nrows, 0);

    size_t col_margin = 0;
    size_t row_margin = 0;
    for (size_t i_block_row = 0; i_block_row < blocks.size(); ++i_block_row) {

        for (size_t irow = 0; irow < block_n_rows; ++irow) {

            for (size_t i_block_col = 0; i_block_col < blocks[i_block_row].size(); ++i_block_col) {
                const CsrMatrix* block = blocks[i_block_row][i_block_col];

                if (block) {
                    // n_nonzeros in row
                    size_t nz = block->addr[irow + 1] - block->addr[irow];
                    n_row_nonzeros[irow + row_margin] += nz;

                    // values
                    const double* v = &block->vals[block->addr[irow]];
                    vals.insert(vals.end(), v, v + nz);

                    // columns
                    const size_t* c = &block->cols[block->addr[irow]];
                    for (size_t i = 0; i < nz; ++i)
                        cols.push_back(c[i] + col_margin);
                }

                col_margin += block_n_cols;
            }
            col_margin = 0;
        }
        row_margin += block_n_rows;
    }

    // assemble addr with running sum
    std::vector<size_t> addr(nrows + 1, 0);
    std::partial_sum(n_row_nonzeros.begin(), n_row_nonzeros.end(), addr.begin() + 1);

    return CsrMatrix(std::move(addr), std::move(cols), std::move(vals));
}

CsrMatrix cfd::assemble_block_matrix(const std::vector<std::vector<const CsrMatrix*>>& blocks) {

    std::vector<size_t> block_n_rows;
    std::vector<size_t> block_n_cols;
    size_t nrows = 0;
    for (auto& b: blocks) {
        bool found = false;
        for (auto& m: b) {
            if (m != nullptr) {
                nrows += m->n_rows();
                block_n_rows.push_back(m->n_rows());
                block_n_cols.push_back(m->n_cols());
                found = true;
                break;
            }
        }
        if (!found) {
            throw std::runtime_error("At least one matrix per block row should be defined");
        }
    }
    std::vector<size_t> cols;
    std::vector<double> vals;
    std::vector<size_t> n_row_nonzeros(nrows, 0);

    size_t col_margin = 0;
    size_t row_margin = 0;
    for (size_t i_block_row = 0; i_block_row < blocks.size(); ++i_block_row) {

        for (size_t irow = 0; irow < block_n_rows[i_block_row]; ++irow) {

            for (size_t i_block_col = 0; i_block_col < blocks[i_block_row].size(); ++i_block_col) {
                const CsrMatrix* block = blocks[i_block_row][i_block_col];

                if (block) {
                    // n_nonzeros in row
                    size_t nz = block->addr[irow + 1] - block->addr[irow];
                    n_row_nonzeros[irow + row_margin] += nz;

                    // values
                    const double* v = &block->vals[block->addr[irow]];
                    vals.insert(vals.end(), v, v + nz);

                    // columns
                    const size_t* c = &block->cols[block->addr[irow]];
                    for (size_t i = 0; i < nz; ++i)
                        cols.push_back(c[i] + col_margin);
                }

                col_margin += block_n_cols[i_block_col];
            }
            col_margin = 0;
        }
        row_margin += block_n_rows[i_block_row];
    }

    // assemble addr with running sum
    std::vector<size_t> addr(nrows + 1, 0);
    std::partial_sum(n_row_nonzeros.begin(), n_row_nonzeros.end(), addr.begin() + 1);

    return CsrMatrix(std::move(addr), std::move(cols), std::move(vals));
}

CsrMatrix cfd::assemble_block_matrix(size_t n_block_rows, size_t n_block_cols,
                                     const std::vector<std::vector<const LodMatrix*>>& blocks) {
    std::vector<CsrMatrix> data;
    size_t nblocks = 0;
    for (const auto& r: blocks) {
        nblocks += r.size();
    };
    data.reserve(nblocks);

    std::vector<std::vector<const CsrMatrix*>> ret(blocks.size());

    for (size_t i = 0; i < blocks.size(); ++i) {
        for (size_t j = 0; j < blocks[i].size(); ++j) {
            const LodMatrix* block = blocks[i][j];

            if (block) {
                data.push_back(block->to_csr());
                ret[i].push_back(&data.back());
            } else {
                ret[i].push_back(nullptr);
            }
        }
    }

    return assemble_block_matrix(n_block_rows, n_block_cols, ret);
}

void CsrMatrix::set_stencil(std::vector<size_t>&& addr_vec, std::vector<size_t>&& cols_vec) {
    CsrStencil::set_stencil(std::move(addr_vec), std::move(cols_vec));
    vals = std::vector<double>(cols.size(), 0.0);
}

void CsrMatrix::set_stencil(const std::vector<std::set<size_t>>& stencil_set) {
    CsrStencil::set_stencil(stencil_set);
    vals = std::vector<double>(cols.size(), 0.0);
}

void CsrMatrix::set_stencil(const CsrStencil& stencil) {
    CsrStencil::set_stencil(stencil);
    vals = std::vector<double>(cols.size(), 0.0);
}

double CsrMatrix::row_sum(size_t irow) const {
    double ret = 0;
    const std::vector<size_t>& a = addr;

    const size_t start = a.at(irow);
    const size_t end = a.at(irow + 1);
    for (size_t i = start; i < end; ++i) {
        ret += vals[i];
    }

    return ret;
}

void CsrMatrix::clear_row(size_t irow) {
    const size_t start = addr.at(irow);
    const size_t end = addr.at(irow + 1);
    for (size_t i = start; i < end; ++i) {
        vals[i] = 0;
    }
}

void CsrMatrix::add_value(size_t irow, size_t icol, double value) {
    size_t a = get_address(irow, icol);
    if (a != INVALID_INDEX) {
        vals[a] += value;
    } else {
        throw std::runtime_error("Invalid index");
    }
}

void CsrMatrix::set_value(size_t irow, size_t icol, double value) {
    size_t a = get_address(irow, icol);
    if (a != INVALID_INDEX) {
        vals[a] = value;
    } else {
        throw std::runtime_error("Invalid index");
    }
}

DenseMatrix CsrMatrix::to_dense() const {
    const size_t n = n_rows();
    std::vector<double> ret(n * n, 0.0);
    for (auto [i, j, v]: matrix_iter::ijv(*this)) {
        ret[j + i * n] = v;
    }
    return DenseMatrix(n, n, ret);
}

CsrMatrix cfd::mat_multiply(const CsrMatrix& A, const CsrMatrix& B) {
    // A rows
    size_t m = A.n_rows();
    // B columns
    size_t n = *std::max_element(B.cols.begin(), B.cols.end()) + 1;

    // Результирующая матрица
    std::vector<size_t> C_addr;
    std::vector<size_t> C_cols;
    std::vector<double> C_vals;

    // Временные массивы для накопления одной строки
    std::vector<double> row_vals(n, 0.0);
    std::vector<int> row_mark(n, -1); // для отметки какие колонки не нулевые
    std::vector<size_t> row_cols;

    C_addr.resize(m + 1, 0);

    // Проходим по строкам A
    for (size_t i = 0; i < m; ++i) {
        row_cols.clear();

        // Для каждого ненуля в строке A
        for (size_t ja = A.addr[i]; ja < A.addr[i + 1]; ++ja) {
            size_t col_a = A.cols[ja]; // k
            double val_a = A.vals[ja];

            // Умножаем на строку col_a из B (внимание: B в CSR, поэтому это строка col_a матрицы B)
            for (size_t jb = B.addr[col_a]; jb < B.addr[col_a + 1]; ++jb) {
                size_t col_b = B.cols[jb]; // j
                double val_b = B.vals[jb];

                double prod = val_a * val_b;

                if (row_mark[col_b] == -1) {
                    // Первый раз встречаем эту колонку
                    row_mark[col_b] = (int)row_cols.size();
                    row_cols.push_back(col_b);
                    row_vals[col_b] = prod;
                } else {
                    // Уже есть вклад в эту колонку
                    row_vals[col_b] += prod;
                }
            }
        }

        // Сортируем колонки по возрастанию (требование для CSR)
        std::sort(row_cols.begin(), row_cols.end());

        // Записываем строку в C
        for (size_t col: row_cols) {
            C_cols.push_back(col);
            C_vals.push_back(row_vals[col]);
            row_vals[col] = 0.0; // очищаем
            row_mark[col] = -1;  // сбрасываем маркер
        }

        C_addr[i + 1] = C_vals.size();
    }

    return CsrMatrix(std::move(C_addr), std::move(C_cols), std::move(C_vals));
}

CsrMatrix cfd::mat_transpose(const CsrMatrix& A) {
    size_t n_rows = A.n_rows();
    size_t n_cols = A.n_cols();
    size_t nnz = A.n_nonzeros();

    const auto& A_addr = A.addr;
    const auto& A_cols = A.cols;
    const auto& A_vals = A.vals;

    // Результат - транспонированная матрица в CSR
    std::vector<size_t> At_addr;
    std::vector<size_t> At_cols;
    std::vector<double> At_vals;

    // Шаг 1: Подсчитываем количество элементов в каждой строке At
    // (строки At = колонки A)
    At_addr.assign(n_cols + 1, 0);

    for (size_t i = 0; i < nnz; ++i) {
        size_t col = A_cols[i];
        At_addr[col + 1]++; // увеличиваем счетчик для следующей строки At
    }

    // Шаг 2: Префиксные суммы - получаем начала строк
    for (size_t i = 0; i < n_cols; ++i) {
        At_addr[i + 1] += At_addr[i];
    }

    // Шаг 3: Заполняем At_cols и At_vals
    At_cols.resize(nnz);
    At_vals.resize(nnz);

    // Вектор для отслеживания текущей позиции в каждой строке At
    std::vector<size_t> position = At_addr; // копия начал строк

    // Проходим по строкам исходной A
    for (size_t row = 0; row < n_rows; ++row) {
        for (size_t j = A_addr[row]; j < A_addr[row + 1]; ++j) {
            size_t col = A_cols[j]; // колонка A = строка At
            double val = A_vals[j];

            // Вставляем в At
            size_t pos = position[col];
            At_cols[pos] = row; // индекс колонки At = строка A
            At_vals[pos] = val;
            position[col]++; // двигаем указатель в этой строке
        }
    }

    return CsrMatrix(std::move(At_addr), std::move(At_cols), std::move(At_vals));
}
