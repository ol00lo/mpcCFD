cmake_minimum_required(VERSION 3.28)

project(grav2)

set(CMAKE_CXX_STANDARD 23)
include(FetchContent)

set(WARN_FLAGS
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
foreach(flag ${WARN_FLAGS})
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:${flag}>")
endforeach()

# Catch framework
find_package(Catch2 3 REQUIRED)

# build all binaries to build/bin
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

message(STATUS "PROJECT_ROOT = ${PROJECT_ROOT}")

set(SRC
    "${PROJECT_ROOT}/test.cpp"
    "${PROJECT_ROOT}/grav2_common.cpp"
	"${PROJECT_ROOT}/../common/tictoc.cpp"
)

message(STATUS "WITH_CUDA = '${WITH_CUDA}'")

if (WITH_CUDA)
    include(CheckLanguage)
    check_language(CUDA)
    if(NOT CMAKE_CUDA_COMPILER)
        message(FATAL_ERROR "WITH_CUDA=ON, but nvcc not found. Set CMAKE_CUDA_COMPILER or CUDACXX.")
    endif()

    set(CUDA_EXCLUDED_FLAGS
        -Wpedantic
        -Wzero-as-null-pointer-constant
        -Wold-style-cast
        -Wsign-promo
        -Werror
        -Wredundant-decls
    )

    foreach(flag ${WARN_FLAGS})
        if(NOT flag IN_LIST CUDA_EXCLUDED_FLAGS)
        add_compile_options("$<$<COMPILE_LANGUAGE:CUDA>:--compiler-options=${flag}>")
        endif()
    endforeach()

    enable_language(CUDA)

    list(APPEND SRC grav2_gpu.cu)

    set(CMAKE_CUDA_STANDARD 20)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
else()
    list(APPEND SRC ../grav2_cpu.cpp)
endif()

add_executable(
    grav2
    ${SRC}
)

target_include_directories(grav2 PRIVATE
    "${PROJECT_ROOT}"
    "${PROJECT_ROOT}/../common"
)

target_link_libraries(grav2 PRIVATE Catch2::Catch2WithMain)
target_compile_definitions(grav2 PRIVATE 
    OUTPUT_BINARY_DIR="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)
