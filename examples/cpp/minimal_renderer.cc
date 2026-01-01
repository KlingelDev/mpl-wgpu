// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

#include "minimal_renderer.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace mpl_wgpu {

MinimalRenderer::MinimalRenderer(unsigned int width, unsigned int height)
    : width_(width), height_(height) {
  pixel_buffer_.resize(width * height * 4, 0);  // RGBA
}

void MinimalRenderer::DrawRects(const std::vector<Rect>& rects,
                                float screen_width,
                                float screen_height) {
  std::cout << "DrawRects: " << rects.size() << " rectangles\n";
  // TODO: Actual GPU rendering
  // For now, simple software rasterization for testing
  for (const auto& rect : rects) {
    int x = static_cast<int>(rect.x);
    int y = static_cast<int>(rect.y);
    int w = static_cast<int>(rect.width);
    int h = static_cast<int>(rect.height);
    
    uint8_t r = static_cast<uint8_t>(rect.r * 255);
    uint8_t g = static_cast<uint8_t>(rect.g * 255);
    uint8_t b = static_cast<uint8_t>(rect.b * 255);
    uint8_t a = static_cast<uint8_t>(rect.a * 255);

    for (int py = y; py < y + h && py < static_cast<int>(height_); 
         ++py) {
      for (int px = x; px < x + w && px < static_cast<int>(width_); 
           ++px) {
        if (px >= 0 && py >= 0) {
          size_t idx = (py * width_ + px) * 4;
          pixel_buffer_[idx + 0] = r;
          pixel_buffer_[idx + 1] = g;
          pixel_buffer_[idx + 2] = b;
          pixel_buffer_[idx + 3] = a;
        }
      }
    }
  }
}

void MinimalRenderer::DrawLines(const std::vector<Line>& lines,
                                float screen_width,
                                float screen_height) {
  std::cout << "DrawLines: " << lines.size() << " lines\n";
  // TODO: Actual GPU line rendering
}

void MinimalRenderer::DrawCircles(const std::vector<Circle>& circles,
                                  float screen_width,
                                  float screen_height) {
  std::cout << "DrawCircles: " << circles.size() << " circles\n";
  // TODO: Actual GPU circle rendering
}

void MinimalRenderer::DrawTriangles(
    const std::vector<Triangle>& triangles, float screen_width,
    float screen_height) {
  std::cout << "DrawTriangles: " << triangles.size() 
            << " triangles\n";
  // TODO: Actual GPU triangle rendering
}

void MinimalRenderer::DrawText(const std::string& text, float x, 
                               float y, float font_size,
                               const std::array<float, 4>& color,
                               float rotation) {
  std::cout << "DrawText: '" << text << "' at (" << x << "," << y 
            << ")\n";
  // TODO: Actual GPU text rendering
}

void MinimalRenderer::Clear(const std::array<float, 4>& color) {
  uint8_t r = static_cast<uint8_t>(color[0] * 255);
  uint8_t g = static_cast<uint8_t>(color[1] * 255);
  uint8_t b = static_cast<uint8_t>(color[2] * 255);
  uint8_t a = static_cast<uint8_t>(color[3] * 255);

  for (size_t i = 0; i < pixel_buffer_.size(); i += 4) {
    pixel_buffer_[i + 0] = r;
    pixel_buffer_[i + 1] = g;
    pixel_buffer_[i + 2] = b;
    pixel_buffer_[i + 3] = a;
  }
}

void MinimalRenderer::DrawImage(const std::vector<float>& data,
                                size_t img_width, size_t img_height,
                                float x, float y, float width,
                                float height) {
  std::cout << "DrawImage: " << img_width << "x" << img_height 
            << "\n";
  // TODO: Actual GPU image rendering
}

const uint8_t* MinimalRenderer::GetPixels() const {
  return pixel_buffer_.data();
}

size_t MinimalRenderer::GetPixelCount() const {
  return width_ * height_;
}

}  // namespace mpl_wgpu
