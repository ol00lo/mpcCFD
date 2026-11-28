add_executable(mpi_test
    mpi_test.cpp
    mpi_reduce_test.cpp
    mpi_schwarz_test.cpp
	mpi_test_common.hpp
    mpi_distributed_test.cpp
    mpi_algebraic_schwarz_test.cpp
    utils/distributed_vector.cpp
    utils/distributed_mat_solver.cpp
)

target_link_libraries(mpi_test
    PRIVATE
        Catch2::Catch2
        MPI::MPI_CXX
        mpc_options
        mpc_common
        cfd
)

target_compile_definitions(mpi_test
    PRIVATE
        TEST_DATA_DIR="${PROJECT_SOURCE_DIR}/src/tests/06_ddm/test_data"
)