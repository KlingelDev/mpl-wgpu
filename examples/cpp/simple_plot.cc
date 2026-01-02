// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

/// Simple example with window display using GLFW

#include "minimal_renderer.h"
#include <matplot/backend/wgpu_backend.h>
#include <matplot/matplot.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// OpenGL headers
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

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

  // Create window with OpenGL context for display
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  #ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  #endif
  
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

  std::cout << "Plot rendered! Setting up display...\n";

  // Make OpenGL context current
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync

  // Create OpenGL texture for pixel buffer
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  std::cout << "Display ready! Press ESC to close\n";

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    // Check for ESC key
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    // Upload pixel buffer to texture
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, renderer->GetPixels());

    // Clear and render full-screen quad with texture
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Use immediate mode for simplicity (works in compatibility profile)
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f,  1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f,  1.0f);
    glEnd();
    
    glDisable(GL_TEXTURE_2D);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glDeleteTextures(1, &texture);

  glfwDestroyWindow(window);
  glfwTerminate();

  std::cout << "Example complete!\n";
  return 0;
}
