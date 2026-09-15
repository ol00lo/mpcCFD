cmake_minimum_required(VERSION 3.20)

project(mpcCFD LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Catch2 3 REQUIRED)

file(GLOB TASK_SOURCES
    CONFIGURE_DEPENDS
    "src/*/*/*.cpp"
)

add_executable(multithreading_tests
    ${TASK_SOURCES}
)

target_link_libraries(multithreading_tests
    PRIVATE
        Catch2::Catch2WithMain
)

target_compile_options(multithreading_tests
    PRIVATE
    -Werror
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wunused-const-variable
    -Wduplicated-cond
    -Wnon-virtual-dtor
    -Woverloaded-virtual
    -Wredundant-decls
    -Wsign-promo
    -Wzero-as-null-pointer-constant
    -Wsuggest-override
    -Wshadow
    -Wfloat-equal
    -Wcast-qual
    -Wlogical-op
    -Wcast-align
    -Wold-style-cast
    -Wmissing-field-initializers
)