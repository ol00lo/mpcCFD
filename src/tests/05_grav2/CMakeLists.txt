option(MPC_ENABLE_CUDA "Build grav2 with CUDA implementation" OFF)

if(MPC_ENABLE_CUDA)
    include(CheckLanguage)
    check_language(CUDA)

    if(NOT CMAKE_CUDA_COMPILER)
        message(FATAL_ERROR "CUDA compiler not found")
    endif()

    enable_language(CUDA)
    find_package(CUDAToolkit REQUIRED)
endif()

add_library(grav2 STATIC
    grav2_common.cpp
)

if(MPC_ENABLE_CUDA)
    target_sources(grav2 PRIVATE grav2_gpu.cu)

    target_link_libraries(grav2 PUBLIC CUDA::cudart)

    set_target_properties(grav2 PROPERTIES
        CUDA_STANDARD 20
        CUDA_STANDARD_REQUIRED ON
    )

    target_compile_definitions(grav2 PUBLIC MPC_HAS_CUDA=1)
else()
    target_sources(grav2 PRIVATE grav2_cpu.cpp)

    target_compile_definitions(grav2 PUBLIC MPC_HAS_CUDA=0)
endif()

target_include_directories(grav2
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(grav2
    PRIVATE
        OUTPUT_BINARY_DIR="${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

target_link_libraries(grav2
    PUBLIC
        mpc_common
        mpc_options
)