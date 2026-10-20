#include <catch2/catch_all.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <locale>
#include <memory>
#include <queue>
#include <random>
#include <thread>
#include <vector>

constexpr double EPS = 1e-12;
constexpr size_t REPORT_ITER = 10;
constexpr size_t MAX_ITER = 1'000;
constexpr size_t N_THREADS = 8;

std::vector<std::vector<double>> build_matrix(size_t N, int seed = 0) {
    std::vector<std::vector<double>> M(N, std::vector<double>(N, 0));
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> VAL(0.0, 1.0);
    std::uniform_real_distribution<double> DIAG_MULT(1.0, 1.3);

    for (size_t irow = 0; irow < N; ++irow) {
        double row_sum = 0.0;
        for (size_t icol = 0; icol < N; ++icol) {
            if (icol != irow) {
                M[irow][icol] = VAL(gen);
                row_sum += std::abs(M[irow][icol]);
            }
        }
        M[irow][irow] = row_sum * DIAG_MULT(gen);
    }

    return M;
}

std::vector<double> build_rhs(size_t N, int seed = 0) {
    std::vector<double> f(N, 0);
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> VAL(-1.0, 1.0);

    for (size_t irow = 0; irow < N; ++irow) {
        f[irow] = VAL(gen);
    }

    return f;
}

double compute_residual(const std::vector<std::vector<double>>& A, const std::vector<double>& f,
                        const std::vector<double>& x) {
    double s = 0;
    for (size_t irow = 0; irow < A.size(); ++irow) {
        double row_r = f[irow];
        for (size_t icol = 0; icol < A.size(); ++icol) {
            row_r -= A[irow][icol] * x[icol];
        }
        s += row_r * row_r;
    }
    return std::sqrt(s / static_cast<double>(A.size()));
}

bool stop_by_residual(size_t iter, double residual) {
    if (residual < EPS) {
        std::cout << "Converged in " << iter << " iterations" << std::endl;
        return true;
    }
    if (iter >= MAX_ITER) {
        std::cout << "FAILED TO CONVERGE in " << MAX_ITER << " iteration "
                  << "with final residual = " << residual << std::endl;
        return true;
    }

    // report in each [K*REPORT_ITER, (K+1)*REPORT_ITER] window once
    // works for non sequential iter-s (actual for async runs)
    static size_t last_reported_window = std::numeric_limits<size_t>::max();
    size_t window = iter / REPORT_ITER;
    if (window != last_reported_window) {
        last_reported_window = window;
        std::cout << "iter = " << iter << ", residual = " << residual << std::endl;
    }
    return false;
}

// JACOBI
void solve_jacobi_serial(const std::vector<std::vector<double>>& A, const std::vector<double>& f,
                         std::vector<double>& x) {
    std::cout << "=== Jacobi serial. N = " << A.size() << std::endl;
    double residual_sum = std::numeric_limits<double>::max();

    for (size_t iter = 0; iter < MAX_ITER + 1; ++iter) {

        // compute residual for previous iteration to avoid
        // superfluous loop in the residual computation
        double r = std::sqrt(residual_sum / static_cast<double>(A.size()));
        if (stop_by_residual(iter, r)) {
            return;
        }

        std::vector<double> x_old(x);
        residual_sum = 0.0;
        for (size_t irow = 0; irow < A.size(); ++irow) {
            double s = f[irow];
            for (size_t icol = 0; icol < irow; ++icol) {
                s -= A[irow][icol] * x_old[icol];
            }
            for (size_t icol = irow + 1; icol < A.size(); ++icol) {
                s -= A[irow][icol] * x_old[icol];
            }
            double diff = s - A[irow][irow] * x_old[irow];
            residual_sum += diff * diff;

            x[irow] = s / A[irow][irow];
        }
    }
}

void solve_jacobi_parallel(const std::vector<std::vector<double>>& A, const std::vector<double>& f,
                           std::vector<double>& x) {
    std::cout << "=== Jacobi parallel. N = " << A.size() << std::endl;
    const size_t chunk_size = static_cast<size_t>(std::ceil(static_cast<double>(A.size()) / N_THREADS));
    std::vector<double> x_old(x);
    std::atomic<double> residual_sum{std::numeric_limits<double>::max()};

    auto worker = [&](size_t istart, size_t iend) {
        double residual_sum_local = 0;
        for (size_t irow = istart; irow < iend; ++irow) {
            double s = f[irow];
            for (size_t icol = 0; icol < irow; ++icol) {
                s -= A[irow][icol] * x_old[icol];
            }
            for (size_t icol = irow + 1; icol < A.size(); ++icol) {
                s -= A[irow][icol] * x_old[icol];
            }
            double diff = s - A[irow][irow] * x_old[irow];
            residual_sum_local += diff * diff;
            x[irow] = s / A[irow][irow];
        }
        residual_sum.fetch_add(residual_sum_local, std::memory_order_relaxed);
    };

    for (size_t iter = 0; iter < MAX_ITER + 1; ++iter) {
        double r = std::sqrt(residual_sum.load(std::memory_order_relaxed) / static_cast<double>(A.size()));
        if (stop_by_residual(iter, r)) {
            return;
        }

        std::swap(x_old, x);
        residual_sum.store(0.0, std::memory_order_relaxed);
        std::vector<std::thread> threads;
        for (size_t ithread = 0; ithread < N_THREADS; ++ithread) {
            size_t istart = ithread * chunk_size;
            size_t iend = std::min(A.size(), istart + chunk_size);
            threads.emplace_back(worker, istart, iend);
        }
        for (auto& t: threads) {
            t.join();
        }
    }
}

class ThreadPool {
    // TODO
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
};

void solve_jacobi_pool(const std::vector<std::vector<double>>&, const std::vector<double>&, std::vector<double>&) {}

// SEIDEL
void solve_seidel_serial(const std::vector<std::vector<double>>& A, const std::vector<double>& f,
                         std::vector<double>& x) {
    std::cout << "=== Seidel serial. N = " << A.size() << std::endl;
    for (size_t iter = 0; iter < MAX_ITER + 1; ++iter) {
        double r = compute_residual(A, f, x);
        if (stop_by_residual(iter, r)) {
            return;
        }

        for (size_t irow = 0; irow < A.size(); ++irow) {
            double s = f[irow];
            for (size_t icol = 0; icol < irow; ++icol) {
                s -= A[irow][icol] * x[icol];
            }
            for (size_t icol = irow + 1; icol < A.size(); ++icol) {
                s -= A[irow][icol] * x[icol];
            }
            x[irow] = s / A[irow][irow];
        }
    }
};

void solve_seidel_async(const std::vector<std::vector<double>>&, const std::vector<double>&, std::vector<double>&) {
    // TODO
}

// output with thousand separator
struct ApostropheSep : std::numpunct<char> {
    char do_thousands_sep() const override {
        return '\'';
    }
    std::string do_grouping() const override {
        return "\3";
    }
};

TEST_CASE("parallel iterative solvers ", "[par_iter]") {
    std::locale loc(std::locale(), new ApostropheSep);
    std::cout.imbue(loc);

    const size_t N = 1'000;

    std::vector<std::vector<double>> A = build_matrix(N);
    std::vector<double> f = build_rhs(N);
    std::vector<double> x(N, 0.0);

    auto start = std::chrono::steady_clock::now();

    solve_jacobi_serial(A, f, x);

    auto end = std::chrono::steady_clock::now();
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "Processed in " << us << " microseconds" << std::endl;

    CHECK(compute_residual(A, f, x) == Catch::Approx(0)).epsilon(1e-9);
}