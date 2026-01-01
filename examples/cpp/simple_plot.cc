// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

// Simple example showing how to use mpl-wgpu backend

#include <matplot/backend/wgpu_backend.h>
#include <matplot/matplot.h>

#include <iostream>
#include <memory>
#include <vector>

// Minimal WgpuRenderer implementation for demonstration
class MinimalRenderer : public matplot::backend::WgpuRenderer {
 public:
  void DrawRects(const std::vector<Rect>& rects, float screen_width,
                 float screen_height) override {
    std::cout << "Drawing " << rects.size() << " rectangles\n";
  }

  void DrawLines(const std::vector<Line>& lines, float screen_width,
                 float screen_height) override {
    std::cout << "Drawing " << lines.size() << " lines\n";
  }

  void DrawCircles(const std::vector<Circle>& circles,
                   float screen_width, float screen_height) override {
    std::cout << "Drawing " << circles.size() << " circles\n";
  }

  void DrawTriangles(const std::vector<Triangle>& triangles,
                     float screen_width,
                     float screen_height) override {
    std::cout << "Drawing " << triangles.size() << " triangles\n";
  }

  void DrawText(const std::string& text, float x, float y,
                float font_size,
                const std::array<float, 4>& color,
                float rotation) override {
    std::cout << "Drawing text: " << text << "\n";
  }

  void Clear(const std::array<float, 4>& color) override {
    std::cout << "Clearing with color\n";
  }

  void DrawImage(const std::vector<float>& data, size_t img_width,
                 size_t img_height, float x, float y, float width,
                 float height) override {
    std::cout << "Drawing image\n";
  }
};

int main() {
  std::cout << "mpl-wgpu Simple Plot Example\n";

  // Create renderer and backend
  auto renderer = std::make_shared<MinimalRenderer>();
  auto backend =
      std::make_shared<matplot::backend::WgpuBackend>(renderer);

  // Create figure and set backend
  auto fig = matplot::figure();
  fig->backend(backend);

  // Create a simple plot
  std::vector<double> x = {1, 2, 3, 4, 5};
  std::vector<double> y = {1, 4, 9, 16, 25};
  
  matplot::plot(x, y);
  matplot::title("Simple Plot Example");
  matplot::xlabel("X Axis");
  matplot::ylabel("Y = X²");

  // Render
  fig->draw();

  std::cout << "Plot rendered successfully!\n";
  return 0;
}
