cmake_minimum_required(VERSION 3.20)
project(mpl-wgpu 
  VERSION 0.1.0
  LANGUAGES CXX
  DESCRIPTION "wgpu rendering backend for matplotplusplus"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(MPL_WGPU_BUILD_TESTS "Build tests" OFF)
option(MPL_WGPU_BUILD_EXAMPLES "Build examples" ON)

# Add matplotplusplus
add_subdirectory(vendor/matplotplusplus)

# Find wgpu (assume wgpu-native or similar is available)
# TODO: Add proper wgpu dependency handling

# Main backend library
add_library(mpl_wgpu_backend
  src/backend/wgpu_backend.cc
  include/matplot/backend/wgpu_backend.h
)

target_include_directories(mpl_wgpu_backend
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/backend
)

target_link_libraries(mpl_wgpu_backend
  PUBLIC
    matplot
  # TODO: Add wgpu library
)

target_compile_options(mpl_wgpu_backend PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

# Examples
if(MPL_WGPU_BUILD_EXAMPLES)
  add_subdirectory(examples/cpp)
endif()

# Tests
if(MPL_WGPU_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# Install rules
install(TARGETS mpl_wgpu_backend
  EXPORT mpl-wgpu-targets  
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/
  DESTINATION include
)
