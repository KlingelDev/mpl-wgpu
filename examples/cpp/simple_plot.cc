// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

/// Simple example with window display using GLFW

#include "minimal_renderer.h"
#include <matplot/backend/wgpu_backend.h>
#include <matplot/matplot.h>

#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <vector>

int main() {
  std::cout << "mpl-wgpu Simple Plot Example (C++)\n";

  // Initialize GLFW
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW\n";
    return 1;
  }

  // Create window
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  const int width = 800;
  const int height = 600;
  GLFWwindow* window = glfwCreateWindow(
      width, height, "mpl-wgpu Example", nullptr, nullptr);
  
  if (!window) {
    std::cerr << "Failed to create window\n";
    glfwTerminate();
    return 1;
  }

  // Create renderer and backend
  auto renderer = std::make_shared<mpl_wgpu::MinimalRenderer>(
      width, height);
  auto backend =
      std::make_shared<matplot::backend::WgpuBackend>(renderer);

  // Create figure and set backend  
  auto fig = matplot::figure();
  fig->backend(backend);
  backend->width(width);
  backend->height(height);

  // Create a simple plot
  std::vector<double> x = {1, 2, 3, 4, 5};
  std::vector<double> y = {1, 4, 9, 16, 25};
  
  matplot::plot(x, y);
  matplot::title("Simple Plot Example");
  matplot::xlabel("X Axis");
  matplot::ylabel("Y = X²");

  // Render once
  fig->draw();

  std::cout << "Plot rendered! Window open, press ESC to close\n";

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // Check for ESC key
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    // TODO: Display pixel buffer to window
    // For now, just keep window open
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  std::cout << "Example complete!\n";
  return 0;
}
