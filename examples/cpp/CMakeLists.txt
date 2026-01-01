cmake_minimum_required(VERSION 3.20)

# Simple line plot example
add_executable(simple_plot simple_plot.cc)
target_link_libraries(simple_plot mpl_wgpu_backend)
