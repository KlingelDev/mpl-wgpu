cmake_minimum_required(VERSION 3.20)

# Find dependencies
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

# Minimal rendering test (no plotting, just basic shapes)
add_executable(test_render
  test_render.cc
  minimal_renderer.cc
  minimal_renderer.h
)
target_link_libraries(test_render mpl_wgpu_backend)

# Simple line plot example with window
add_executable(simple_plot 
  simple_plot.cc
  minimal_renderer.cc
  minimal_renderer.h
)
target_link_libraries(simple_plot 
  mpl_wgpu_backend
  glfw
  OpenGL::GL
)

add_executable(gpu_plot
  gpu_plot.cc
  window.cc
  window.h
  ../../src/backend/primitive_renderer.cc # Need to compile this too if not in lib
  ../../src/backend/primitive_renderer.h
)
# Ensure we find WebGPU (assumed linked in backend, but we need headers)
# For now, linking backend might be enough if it exposes includes
target_include_directories(gpu_plot PRIVATE 
  ../../src/backend 
  ../../include
)
target_link_libraries(gpu_plot
  mpl_wgpu_backend
  glfw
  webgpu
)

add_executable(test_gallery
  test_gallery.cc
  window.cc
  window.h
  ../../src/backend/primitive_renderer.cc
  ../../src/backend/primitive_renderer.h
)
target_include_directories(test_gallery PRIVATE
  ../../src/backend
  ../../include
)
target_link_libraries(test_gallery
  mpl_wgpu_backend
  glfw
  webgpu
)

add_executable(headless_test
  headless_test.cc
  test_cases.cc
  image_utils.cc
  ../../src/backend/frame_capture.cc
  ../../src/backend/primitive_renderer.cc
  ../../src/backend/primitive_renderer.h
)
target_include_directories(headless_test PRIVATE
  ../../src/backend
  ../../include
)
target_link_libraries(headless_test
  mpl_wgpu_backend
  webgpu
)
