cmake_minimum_required(VERSION 3.15)
project(mpl-wgpu VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(MPL_WGPU_BUILD_TESTS "Build tests" OFF)
option(MPL_WGPU_BUILD_EXAMPLES "Build examples" ON)

# Add matplotplusplus
add_subdirectory(vendor/matplotplusplus)

# wgpu backend library
add_library(mpl_wgpu_backend
    cpp/backend/wgpu_backend.cpp
    cpp/backend/wgpu_backend.h
)

target_include_directories(mpl_wgpu_backend
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cpp>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/cpp/backend
)

target_link_libraries(mpl_wgpu_backend
    PUBLIC
        matplot
)

# TODO: Link wgpu (via wgpu-native or wgpu-rs)
# For now, this is C++ skeleton only

# Install rules
install(TARGETS mpl_wgpu_backend
    EXPORT mpl-wgpu-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES cpp/backend/wgpu_backend.h
    DESTINATION include/mpl_wgpu
)
