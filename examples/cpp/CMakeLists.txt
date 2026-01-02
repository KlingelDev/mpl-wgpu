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
