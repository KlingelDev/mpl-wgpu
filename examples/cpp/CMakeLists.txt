cmake_minimum_required(VERSION 3.20)

# Find GLFW
find_package(glfw3 REQUIRED)

# Simple line plot example with window
add_executable(simple_plot 
  simple_plot.cc
  minimal_renderer.cc
  minimal_renderer.h
)
target_link_libraries(simple_plot 
  mpl_wgpu_backend
  glfw
)
