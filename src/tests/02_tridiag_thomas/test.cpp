#include <catch2/catch_all.hpp>

#include <barrier>
#include <iostream>
#include <thread>
#include <vector>

struct TridiagMatrix {
    int n;
    std::vector<double> a; // a[0] unused
    std::vector<double> b;
    std::vector<double> c; // c[n-1] unused
};

TridiagMatrix create_tridiag_matrix(int n) {
    TridiagMatrix ret;
    ret.n = n;
    ret.a = std::vector<double>(n, -1.0);
    ret.b = std::vector<double>(n, 2.0);
    ret.c = std::vector<double>(n, -1.0);
    ret.a[0] = 0.0;
    ret.b[0] = 1.0;
    ret.c[0] = 0.0;
    ret.a[n - 1] = 0.0;
    ret.b[n - 1] = 1.0;
    ret.c[n - 1] = 0.0;
    return ret;
}

std::vector<double> thomas(const TridiagMatrix& m, const std::vector<double>& d) {
    const int n = m.n;

    if (static_cast<int>(d.size()) != n)
        throw std::invalid_argument("d.size() != n");
    if (n == 0)
        return {};
    if (std::abs(m.b[0]) < 1e-16)
        throw std::runtime_error("zero b[0]: thomas method not applicable");

    // TODO

    return {};
}

std::vector<double> thomas_range(int start, int end, const TridiagMatrix& m, const std::vector<double>& d,
                                 double left_bc, double right_bc) {
    if (static_cast<int>(d.size()) != m.n)
        throw std::invalid_argument("d.size() != m.n");
    if (m.n == 0)
        return {};
    if (std::abs(m.b[start]) < 1e-16)
        throw std::runtime_error("zero b[0]: thomas method not applicable");

    const int n = end - start;

    // TODO

    return {};
}

std::vector<double> thomas_parallel(const TridiagMatrix& m, const std::vector<double>& d, int nthreads) {
    std::vector<double> x_old(m.n, 0.0);
    std::vector<double> x_new = x_old;
    std::vector<std::thread> threads;

    // TODO
    return {};
}

double residual(const TridiagMatrix& m, const std::vector<double>& x, const std::vector<double>& f) {
    std::vector<double> r(m.n, 0.0);
    for (int i = 0; i < m.n; i++) {
        r[i] = -m.b[i] * x[i] + f[i];
        if (i > 0)
            r[i] -= m.a[i] * x[i - 1];
        if (i < m.n - 1)
            r[i] -= m.c[i] * x[i + 1];
    }

    double res = 0.0;
    for (int i = 0; i < m.n; i++)
        res += (r[i]) * (r[i]);
    return res;
}

TEST_CASE("Parallel tridiagonal solver", "[tridiag]") {
    constexpr int n = 10;
    auto mat = create_tridiag_matrix(n);

    std::vector<double> f(n, 0.0);
    f[0] = 0;
    f[n - 1] = 1.0;

    // auto x = thomas(mat, f);
    auto x = thomas_range(0, n, mat, f, 0.0, 1.0);

    for (auto v: x)
        std::cout << v << std::endl;

    CHECK(x[0] == Catch::Approx(0.0));
    CHECK(x[n - 1] == Catch::Approx(1.0));

    auto x_parallel = thomas_parallel(mat, f, 4);
    CHECK(std::abs(residual(mat, x_parallel, f)) < 1e-10);
}