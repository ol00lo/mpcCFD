#include <catch2/catch_test_macros.hpp>

#include <iostream>
#include <vector>

struct CSRMatrix {
    int rows;
    int cols;

    std::vector<double> values;
    std::vector<int> col_idx;
    std::vector<int> row_ptr;
};

CSRMatrix multiply(const CSRMatrix& A, const CSRMatrix& B) {
    std::cout << B.rows << std::endl;
    return A;
}

TEST_CASE("Multiply CSR matrices", "[csr_multiply]") {
    CSRMatrix A{2, 3, {2.0, 3.0, 4.0}, {0, 2, 1}, {0, 2, 3}};

    CSRMatrix B{3, 2, {1.0, 2.0, 3.0, 4.0}, {0, 1, 0, 1}, {0, 2, 3, 4}};

    const CSRMatrix result = multiply(A, B);

    CHECK(result.rows == 2);
}
