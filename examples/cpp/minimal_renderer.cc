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
  
  float scale_x = static_cast<float>(width_) / screen_width;
  float scale_y = static_cast<float>(height_) / screen_height;
  
  for (const auto& line : lines) {
    // Transform coordinates
    int x0 = static_cast<int>(line.x1 * scale_x);
    int y0 = static_cast<int>(line.y1 * scale_y);
    int x1 = static_cast<int>(line.x2 * scale_x);
    int y1 = static_cast<int>(line.y2 * scale_y);
    
    uint8_t r = static_cast<uint8_t>(line.r * 255);
    uint8_t g = static_cast<uint8_t>(line.g * 255);
    uint8_t b = static_cast<uint8_t>(line.b * 255);
    uint8_t a = static_cast<uint8_t>(line.a * 255);
    
    // Bresenham's line algorithm
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    
    int x = x0, y = y0;
    while (true) {
      // Draw pixel
      if (x >= 0 && x < static_cast<int>(width_) && 
          y >= 0 && y < static_cast<int>(height_)) {
        size_t idx = (y * width_ + x) * 4;
        pixel_buffer_[idx + 0] = r;
        pixel_buffer_[idx + 1] = g;
        pixel_buffer_[idx + 2] = b;
        pixel_buffer_[idx + 3] = a;
      }
      
      if (x == x1 && y == y1) break;
      
      int e2 = 2 * err;
      if (e2 > -dy) {
        err -= dy;
        x += sx;
      }
      if (e2 < dx) {
        err += dx;
        y += sy;
      }
    }
  }
}

void MinimalRenderer::DrawCircles(const std::vector<Circle>& circles,
                                  float screen_width,
                                  float screen_height) {
  std::cout << "DrawCircles: " << circles.size() << " circles\n";
  
  float scale_x = static_cast<float>(width_) / screen_width;
  float scale_y = static_cast<float>(height_) / screen_height;
  
  for (const auto& circle : circles) {
    int cx = static_cast<int>(circle.cx * scale_x);
    int cy = static_cast<int>(circle.cy * scale_y);
    int radius = static_cast<int>(circle.radius);
    
    uint8_t r = static_cast<uint8_t>(circle.r * 255);
    uint8_t g = static_cast<uint8_t>(circle.g * 255);
    uint8_t b = static_cast<uint8_t>(circle.b * 255);
    uint8_t a = static_cast<uint8_t>(circle.a * 255);
    
    // Draw filled circle
    for (int y = cy - radius; y <= cy + radius; ++y) {
      for (int x = cx - radius; x <= cx + radius; ++x) {
        int dx = x - cx;
        int dy = y - cy;
        if (dx * dx + dy * dy <= radius * radius) {
          if (x >= 0 && x < static_cast<int>(width_) &&
              y >= 0 && y < static_cast<int>(height_)) {
            size_t idx = (y * width_ + x) * 4;
            pixel_buffer_[idx + 0] = r;
            pixel_buffer_[idx + 1] = g;
            pixel_buffer_[idx + 2] = b;
            pixel_buffer_[idx + 3] = a;
          }
        }
      }
    }
  }
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
