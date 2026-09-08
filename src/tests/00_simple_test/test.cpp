#include <catch2/catch_test_macros.hpp>

#include <iostream>
#include <thread>

TEST_CASE("Create and join a thread", "[simple_test]") {
    std::thread worker([] { std::cout << "Hello from worker thread!\n"; });

    worker.join();

    std::cout << "Worker thread has finished.\n";

    REQUIRE(true);
}