#define CATCH_CONFIG_RUNNER
#define CATCH_CONFIG_NOSTDOUT
#include "catch2/catch_all.hpp"
#include <catch2/catch_session.hpp>

#include <iostream>
#include <mpi.h>
#include <sstream>

// Catch output from rank-0 only
namespace Catch {
int g_rank = 0;
static std::ostringstream& sink() {
    static std::ostringstream s;
    return s;
}
std::ostream& cout() {
    static std::ostream stream(g_rank == 0 ? std::cout.rdbuf() : sink().rdbuf());
    return stream;
}
std::ostream& clog() {
    static std::ostream stream(g_rank == 0 ? std::clog.rdbuf() : sink().rdbuf());
    return stream;
}
std::ostream& cerr() {
    static std::ostream stream(g_rank == 0 ? std::cerr.rdbuf() : sink().rdbuf());
    return stream;
}
} // namespace Catch

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &Catch::g_rank);

    int result = Catch::Session().run(argc, argv);

    MPI_Finalize();

    if (Catch::g_rank == 0) {
        Catch::cout() << "DONE" << std::endl;
    }
    return result;
}

TEST_CASE("Ping test", "[ping]") {
    int irank;
    MPI_Comm_rank(MPI_COMM_WORLD, &irank);
    std::cout << "Rank " << irank << std::flush << std::endl;
    REQUIRE(true);
}
