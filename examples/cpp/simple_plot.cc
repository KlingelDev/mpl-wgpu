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
#include <fstream>

int main() {
  std::cout << "mpl-wgpu Simple Plot Example (C++)\n";

  // Initialize GLFW
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW\n";
    return 1;
  }

  // Create window with OpenGL context for display
  // Use compatibility profile for immediate mode rendering
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);  // Hide title bar
  
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

  std::cout << "About to call fig->draw()...\n";
  std::cout << "Backend pointer: " << backend.get() << "\n";
  
  // Render once
  fig->draw();

  std::cout << "fig->draw() completed!\n";

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

  // Debug: Check if pixel buffer has data
  const uint8_t* pixels = renderer->GetPixels();
  int non_zero = 0;
  for (size_t i = 0; i < width * height * 4; i++) {
    if (pixels[i] != 0) non_zero++;
  }
  std::cout << "DEBUG: Pixel buffer has " << non_zero << " non-zero bytes out of " 
            << (width * height * 4) << " total\n";

  // Save pixel buffer to file for inspection
  std::ofstream ppm("output.ppm", std::ios::binary);
  ppm << "P6\n" << width << " " << height << "\n255\n";
  for (size_t i = 0; i < width * height; i++) {
    ppm.write(reinterpret_cast<const char*>(&pixels[i*4]), 3); // RGB only
  }
  ppm.close();
  std::cout << "DEBUG: Saved pixel buffer to output.ppm\n";

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    // Check for ESC key
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    // Set viewport to full window
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    // Upload pixel buffer to texture
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, renderer->GetPixels());

    // Set up orthographic projection (pixel coordinates)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);  // Top-left origin
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Draw textured quad filling entire window
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);           // Top-left
    glTexCoord2f(1.0f, 0.0f); glVertex2f((float)width, 0.0f);   // Top-right  
    glTexCoord2f(1.0f, 1.0f); glVertex2f((float)width, (float)height); // Bottom-right
    glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, (float)height);  // Bottom-left
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
