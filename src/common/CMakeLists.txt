add_library(mpc_common STATIC
    tictoc.cpp
)

target_include_directories(mpc_common
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mpc_common
    PUBLIC mpc_options
)