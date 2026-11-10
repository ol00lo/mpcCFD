cmake_minimum_required(VERSION 3.20)

project(mpcCFD LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_library(mpc_options INTERFACE)

target_compile_options(mpc_options
    INTERFACE
        $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU,Clang>>:
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
            -Werror
        >
)

find_package(Catch2 3 REQUIRED)

add_subdirectory(src/common)
add_subdirectory(src/tests/05_grav2)

file(GLOB TASK_SOURCES
    CONFIGURE_DEPENDS
    "src/tests/*/test.cpp"
)

add_executable(multithreading_tests
    ${TASK_SOURCES}
)

target_link_libraries(multithreading_tests
    PRIVATE
        Catch2::Catch2WithMain
        mpc_common
        mpc_options
        grav2
)