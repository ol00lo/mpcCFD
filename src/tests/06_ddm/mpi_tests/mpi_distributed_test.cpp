#include "cfd/mat/lodmat.hpp"
#include "mpi_test_common.hpp"
#include "utils/distributed_mat_solver.hpp"
#include "utils/distributed_vector.hpp"
#include <random>
#include <vector>

using namespace cfd;

namespace {

std::shared_ptr<RankContext> construct_context_1d(size_t nvec, size_t nbuf) {
    int mpi_rank, mpi_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    size_t chunk_size = (nvec + mpi_size - 1) / mpi_size;

    // indexes
    size_t start_index = mpi_rank * chunk_size;
    size_t end_index = std::min(nvec, start_index + chunk_size);
    if (end_index - start_index < 2 * (nbuf + 1)) {
        throw std::runtime_error("Buffer size is too big");
    }
    size_t left_buf_size = std::min(start_index, nbuf + 1);
    size_t right_buf_size = std::min(nvec - end_index, nbuf + 1);
    size_t gindex = start_index - left_buf_size;

    // fill
    std::vector<size_t> global_indexes;
    std::vector<double> Xi;
    // left buffer
    if (left_buf_size > 0) {
        // ghost value
        Xi.push_back(0.0);
        global_indexes.push_back(gindex++);
        // buffer values
        for (size_t i = 1; i < left_buf_size + nbuf; ++i) {
            Xi.push_back(0.5);
            global_indexes.push_back(gindex++);
        }
    }
    // internal
    size_t internal_start = (left_buf_size > 0) ? start_index + nbuf : start_index;
    size_t internal_end = (right_buf_size > 0) ? end_index - nbuf : end_index;
    for (size_t i = internal_start; i < internal_end; ++i) {
        Xi.push_back(1.0);
        global_indexes.push_back(gindex++);
    }
    // right buffer
    if (right_buf_size > 0) {
        // buffer values
        for (size_t i = 1; i < right_buf_size + nbuf; ++i) {
            Xi.push_back(0.5);
            global_indexes.push_back(gindex++);
        }
        // ghost value
        Xi.push_back(0.0);
        global_indexes.push_back(gindex++);
    }

    return std::make_shared<RankContext>(nvec, std::move(Xi), std::move(global_indexes));
};

} // namespace

TEST_CASE("Distributed vector", "[distributed-vec]") {
    constexpr size_t N = 100;
    constexpr size_t NBUFFER = 2;

    // ground truth data
    std::vector<double> gt_vector(N);
    std::mt19937 gen(42);
    std::uniform_real_distribution<double> VAL(0.0, 1.0);
    for (size_t i = 0; i < gt_vector.size(); ++i) {
        gt_vector[i] = VAL(gen);
    }
    double gt_sum = std::accumulate(gt_vector.begin(), gt_vector.end(), 0.0);
    double gt_sqr_sum = 0;
    for (auto v: gt_vector) {
        gt_sqr_sum += (v * v);
    }

    // build current rank context
    int mpi_rank, mpi_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    auto context = construct_context_1d(N, NBUFFER);

    // fill vector
    DistributedVector vec(context);
    for (size_t i = 0; i < vec.size(); ++i) {
        vec.value[i] = gt_vector[context->global_indexes[i]];
    }
    vec.exchange_bc();

    std::vector<double> full_vec = vec.get_full_vector();
    if (mpi_rank == 0) {
        double sum = std::accumulate(full_vec.begin(), full_vec.end(), 0.0);
        double sqr_sum = 0;
        for (auto& v: full_vec) {
            sqr_sum += (v * v);
        }
        CHECK(sum == gt_sum);
        CHECK(sqr_sum == gt_sqr_sum);
    }

    // ALGEBRAIC_SCHWARZ.1
}

TEST_CASE("Distributed matrix solution", "[distributed-mat]") {
    constexpr size_t NRANK = 10;
    constexpr size_t NBUFFER = 2;

    int mpi_rank, mpi_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    // build context
    auto context = construct_context_1d(NRANK * mpi_size, NBUFFER);

    // assemble local matrix
    size_t n_rows = NRANK;
    if (mpi_rank != 0) {
        n_rows += NBUFFER + 1;
    }
    if (mpi_rank != mpi_size - 1) {
        n_rows += NBUFFER + 1;
    }
    LodMatrix lodmat(n_rows);
    lodmat.add_value(0, 0, 1.0);
    lodmat.add_value(n_rows - 1, n_rows - 1, 1.0);
    for (size_t irow = 1; irow < n_rows - 1; ++irow) {
        lodmat.add_value(irow, irow - 1, -1.0);
        lodmat.add_value(irow, irow + 1, -1.0);
        lodmat.add_value(irow, irow, 2.0);
    }

    // assemble local rhs
    DistributedVector rhs(context);
    if (mpi_rank == 0) {
        rhs.value[0] = 1.0; // left bc
    }
    if (mpi_rank == mpi_size - 1) {
        rhs.value.back() = 2.0; // right bc
    }

    // solve
    CsrMatrix csrmat = lodmat.to_csr();

    // ALGEBRAIC_SCHWARZ.2

    // check solution
    std::vector<double> full_u = x.get_full_vector();
    if (mpi_rank == 0) {
        for (size_t i = 0; i < full_u.size(); ++i) {
            CHECK(full_u[i] == Catch::Approx(1.0 + static_cast<double>(i) / (full_u.size() - 1)).margin(1e-4));
        }
    }
}
