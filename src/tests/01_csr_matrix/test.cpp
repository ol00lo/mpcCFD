#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <random>
#include <tuple>
#include <vector>

struct CsrMatrix {
    std::vector<size_t> addr;
    std::vector<size_t> cols;
    std::vector<double> vals;
};

CsrMatrix create_matrix(size_t n, double fill_portion, int seed = 0) {
    size_t nvals = static_cast<size_t>(static_cast<double>(n * n) * fill_portion);
    std::vector<std::tuple<size_t, size_t, double>> values(nvals);

    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> int_dist(0, static_cast<int>(n - 1));
    std::uniform_real_distribution<double> double_dist(0.0, 1000.0);

    for (size_t i = 0; i < nvals; i++)
        values[i] = {int_dist(gen), int_dist(gen), double_dist(gen)};

    auto less = [](const auto& a, const auto& b) {
        if (std::get<0>(a) != std::get<0>(b))
            return std::get<0>(a) < std::get<0>(b);
        return std::get<1>(a) < std::get<1>(b);
    };
    auto equal = [](const auto& a, const auto& b) {
        return std::get<0>(a) == std::get<0>(b) && std::get<1>(a) == std::get<1>(b);
    };

    std::sort(values.begin(), values.end(), less);
    auto it = std::unique(values.begin(), values.end(), equal);
    values.resize(it - values.begin());

    CsrMatrix res;
    size_t current_row = 0;
    res.addr.push_back(0);

    for (auto val: values) {
        auto [r, c, v] = val;

        while (current_row < r) {
            res.addr.push_back(res.vals.size());
            ++current_row;
        }

        res.vals.push_back(v);
        res.cols.push_back(c);
    }
    while (res.addr.size() < n + 1)
        res.addr.push_back(res.vals.size());

    return res;
}

std::vector<double> create_vector(size_t n, int seed = 42) {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> double_dist(0.0, 1000.0);
    std::vector<double> res;

    for (size_t i = 0; i < n; i++)
        res.push_back(double_dist(gen));

    return res;
}

std::vector<double> multiply(const CsrMatrix& m, const std::vector<double>& x);

TEST_CASE("Multiply CSR matrices", "[csr_multiply]") {
    auto m = create_matrix(4, 0.5);
    auto v = create_vector(4);

    // auto res = mult
    CHECK(1 == 1);
}