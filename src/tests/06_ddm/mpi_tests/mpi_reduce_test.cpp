#include "mpi_test_common.hpp"
#include <random>
#include <vector>

TEST_CASE("Reduce vector test", "[reduce]") {
    int mpi_rank, mpi_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    constexpr int N = 1001;

    int chunk_size = (N + mpi_size - 1) / mpi_size;
    int Nmpi = chunk_size * mpi_size;

    std::vector<int> global_vector;
    int true_sum;
    std::vector<int> local_vector(chunk_size);

    // fill global vector on master rank only
    if (mpi_rank == 0) {
        std::mt19937 gen(0);
        std::uniform_int_distribution<int> dist(0, 10);

        // set size dividable by mpi_size.
        // fill with zeros if extra entries are required
        global_vector.resize(Nmpi, 0);

        for (int i = 0; i < N; ++i) {
            global_vector[i] = dist(gen);
        }
        true_sum = std::accumulate(global_vector.begin(), global_vector.end(), 0);
    }

    // send to all ranks
    MPI_Scatter(global_vector.data(), chunk_size, MPI_INT, local_vector.data(), chunk_size, MPI_INT, 0, MPI_COMM_WORLD);

    // compute local vector sum
    int local_sum = std::accumulate(local_vector.begin(), local_vector.end(), 0);

    // 1. reduce to the master rank
    {
        int global_sum;
        MPI_Reduce(&local_sum,    // [in] Указатель на отправляемые данные (на каждом процессе)
                   &global_sum,   // [out] Указатель на буфер для результата (только на root)
                   1,             // [in] Количество элементов в sendbuf
                   MPI_INT,       // [in] Тип данных (MPI_INT, MPI_FLOAT и т. д.)
                   MPI_SUM,       // [in] Операция редукции (MPI_SUM, MPI_MAX и т. д.)
                   0,             // [in] Ранг процесса-получателя результата
                   MPI_COMM_WORLD // [in] Коммуникатор (обычно MPI_COMM_WORLD)
        );

        if (mpi_rank == 0) {
            CHECK(global_sum == true_sum);
        }
    }

    // 2. reduce to all ranks
    {
        // Task 3
    }
}