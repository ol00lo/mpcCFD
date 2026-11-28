#include "distributed_vector.hpp"
#include <set>
#include <unordered_map>

namespace cfd {

namespace {
std::vector<size_t> build_exchange_indexes(const std::vector<size_t>& current_indexes,
                                           const std::vector<size_t>& other_indexes) {
    if (current_indexes.empty() || other_indexes.empty()) {
        return {};
    }

    // global to local indexes map
    std::unordered_map<size_t, size_t> local;
    local.reserve(current_indexes.size());
    for (size_t i = 0; i < current_indexes.size(); ++i) {
        local.emplace(current_indexes[i], i);
    }

    // common global indexes sorted.
    std::vector<size_t> common;
    for (size_t other_i: other_indexes) {
        if (local.find(other_i) != local.end()) {
            common.push_back(other_i);
        }
    }
    std::sort(common.begin(), common.end());

    std::vector<size_t> ret;
    ret.reserve(common.size());
    for (size_t g: common) {
        ret.push_back(local.at(g));
    }

    return ret;
}

} // namespace

Exchanger::Exchanger(int other_rank, std::vector<size_t> indexes)
    : other_rank_(other_rank),
      indexes_(std::move(indexes)) {}

size_t Exchanger::size() const {
    return indexes_.size();
}

MPI_Request Exchanger::post_recv(int tag, ExchangeBuffer& buf) const {
    MPI_Request req;
    MPI_Irecv(buf.recv_buffer.data(), static_cast<int>(buf.recv_buffer.size()), MPI_DOUBLE, other_rank_, tag,
              MPI_COMM_WORLD, &req);
    return req;
}

MPI_Request Exchanger::post_send(int tag, ExchangeBuffer& buf, const std::vector<double>& value) const {
    for (size_t i = 0; i < indexes_.size(); ++i) {
        buf.send_buffer[i] = value[indexes_[i]];
    }
    MPI_Request req;
    MPI_Isend(buf.send_buffer.data(), static_cast<int>(buf.recv_buffer.size()), MPI_DOUBLE, other_rank_, tag,
              MPI_COMM_WORLD, &req);
    return req;
}

void Exchanger::add_buffer(const ExchangeBuffer& buf, std::vector<double>& value) const {
    for (size_t i = 0; i < size(); ++i) {
        value[indexes_[i]] += buf.recv_buffer[i];
    }
}

namespace {

std::vector<size_t> build_owned_rows(const std::vector<double>& blending_weights) {
    std::vector<size_t> ret;
    ret.reserve(blending_weights.size());
    for (size_t i = 0; i < blending_weights.size(); ++i) {
        if (blending_weights[i] > 0) {
            ret.push_back(i);
        }
    }
    return ret;
}

std::vector<size_t> build_ghost_rows(const std::vector<double>& blending_weights) {
    std::vector<size_t> ret;
    for (size_t i = 0; i < blending_weights.size(); ++i) {
        if (blending_weights[i] <= 1e-12) {
            ret.push_back(i);
        }
    }
    return ret;
}

} // namespace

RankContext::RankContext(size_t _global_size, std::vector<double> _blending_weights,
                         std::vector<size_t> _global_indexes)
    : global_size(std::move(_global_size)),
      blending_weights(std::move(_blending_weights)),
      global_indexes(std::move(_global_indexes)),
      owned_rows(build_owned_rows(blending_weights)),
      ghost_rows(build_ghost_rows(blending_weights)) {

    setup_exchangers();
}

void RankContext::setup_exchangers() {
    int mpi_rank, mpi_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    // gather sizes from all ranks
    std::vector<int> sizes(mpi_size);
    int my_size = static_cast<int>(global_indexes.size());
    MPI_Allgather(&my_size, 1, MPI_INT, sizes.data(), 1, MPI_INT, MPI_COMM_WORLD);

    // offsets
    std::vector<int> offsets(mpi_size, 0);
    for (int i = 1; i < mpi_size; ++i) {
        offsets[i] = offsets[i - 1] + sizes[i - 1];
    }
    // get global indexes from all ranks
    std::vector<size_t> all(offsets.back() + sizes.back());
    MPI_Allgatherv(global_indexes.data(), my_size, MPI_UINT64_T, all.data(), sizes.data(), offsets.data(), MPI_UINT64_T,
                   MPI_COMM_WORLD);

    // build exchangers
    for (int irank = 0; irank < mpi_size; ++irank) {
        if (irank == mpi_rank)
            continue;
        std::vector<size_t> other(all.begin() + offsets[irank], all.begin() + offsets[irank] + sizes[irank]);
        auto indexes = build_exchange_indexes(global_indexes, other);
        if (!indexes.empty())
            exchangers.emplace_back(irank, std::move(indexes));
    }
}

void RankContext::invalidate_ghost_slae_rows(CsrMatrix& mat, DistributedVector& rhs) const {
    for (size_t irow: ghost_rows) {
        rhs.value[irow] = 0.0;
        mat.clear_row(irow);
    }
}

std::vector<ExchangeBuffer> RankContext::build_buffer() const {
    std::vector<ExchangeBuffer> ret(exchangers.size());
    for (size_t k = 0; k < exchangers.size(); ++k) {
        const auto& ex = exchangers[k];
        ret[k].send_buffer.resize(ex.size());
        ret[k].recv_buffer.resize(ex.size());
    }
    return ret;
}

int DistributedVector::last_used_tag = 1'000'000;

DistributedVector::DistributedVector(std::shared_ptr<RankContext> rank_context, std::vector<double> data)
    : value(std::move(data)),
      mpi_tag_(last_used_tag++),
      rank_context_(rank_context),
      buffers_(rank_context_->build_buffer()) {

    if (value.empty()) {
        value.resize(rank_context_->global_indexes.size(), 0.0);
    }
    if (value.size() != rank_context_->global_indexes.size()) {
        throw std::runtime_error("Invalid datasize in DistributedVector::DistributedVector");
    }
}

void DistributedVector::exchange_bc() {
    // 1. Use blending_weights
    for (size_t i = 0; i < value.size(); ++i) {
        value[i] *= rank_context_->blending_weights[i];
    }
    std::vector<MPI_Request> reqs;

    // 2. Post receivers
    for (size_t i = 0; i < rank_context_->exchangers.size(); ++i) {
        reqs.push_back(rank_context_->exchangers[i].post_recv(mpi_tag_, buffers_[i]));
    }

    // 3. Post senders
    for (size_t i = 0; i < rank_context_->exchangers.size(); ++i) {
        reqs.push_back(rank_context_->exchangers[i].post_send(mpi_tag_, buffers_[i], value));
    }

    // 4. wait for all exhanges
    MPI_Waitall(static_cast<int>(reqs.size()), reqs.data(), MPI_STATUSES_IGNORE);

    // 5. use input buffers
    for (size_t i = 0; i < rank_context_->exchangers.size(); ++i) {
        rank_context_->exchangers[i].add_buffer(buffers_[i], value);
    }
}

std::vector<double> DistributedVector::get_full_vector() const {
    int mpi_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    std::vector<double> buf(rank_context_->global_size);
    for (size_t i = 0; i < size(); ++i) {
        buf[rank_context_->global_indexes[i]] = rank_context_->blending_weights[i] * value[i];
    }
    void* from = (mpi_rank == 0) ? MPI_IN_PLACE : buf.data();
    MPI_Reduce(from, buf.data(), static_cast<int>(buf.size()), MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    return (mpi_rank == 0) ? buf : std::vector<double>{};
}

std::vector<double> DistributedVector::weighted_value() const {
    std::vector<double> ret(value);
    for (size_t i = 0; i < ret.size(); ++i) {
        ret[i] *= rank_context_->blending_weights[i];
    }
    return ret;
}

} // namespace cfd
