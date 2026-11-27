#ifndef MPC_CFD_MPI_HPP
#define MPC_CFD_MPI_HPP

#include "cfd/mat/csrmat.hpp"
#include "mpi.h"
#include <memory>
#include <vector>

namespace cfd {

struct ExchangeBuffer {
    std::vector<double> recv_buffer;
    std::vector<double> send_buffer;
};

class Exchanger {
public:
    Exchanger(int other_rank, std::vector<size_t> indexes);

    size_t size() const;

    MPI_Request post_recv(int tag, ExchangeBuffer& buf) const;

    MPI_Request post_send(int tag, ExchangeBuffer& buf, const std::vector<double>& value) const;

    void add_buffer(const ExchangeBuffer& buf, std::vector<double>& value) const;

private:
    const int other_rank_;
    const std::vector<size_t> indexes_;
};

class DistributedVector;

struct RankContext {
    RankContext(size_t _global_size, std::vector<double> _blending_weights, std::vector<size_t> _global_indexes);

    const size_t global_size;
    const std::vector<double> blending_weights;
    const std::vector<size_t> global_indexes;
    const std::vector<size_t> owned_rows;
    const std::vector<size_t> ghost_rows;
    std::vector<Exchanger> exchangers;

    std::vector<ExchangeBuffer> build_buffer() const;
    void invalidate_ghost_slae_rows(CsrMatrix& mat, DistributedVector& rhs) const;

private:
    void setup_exchangers();
};

class DistributedVector {
public:
    DistributedVector(std::shared_ptr<RankContext> rank_context, std::vector<double> data = {});

    size_t size() const {
        return value.size();
    }

    void exchange_bc();

    std::vector<double> weighted_value() const;
    std::vector<double> get_full_vector() const;

    std::vector<double> value;

    std::shared_ptr<RankContext> rank_context() const {
        return rank_context_;
    }

private:
    static int last_used_tag;
    const int mpi_tag_;
    std::shared_ptr<RankContext> rank_context_;
    std::vector<ExchangeBuffer> buffers_;
};

} // namespace cfd
#endif
