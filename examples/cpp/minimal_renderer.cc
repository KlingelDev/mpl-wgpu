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
  if (lines.empty()) return;

  float scale_x = static_cast<float>(width_) / screen_width;
  float scale_y = static_cast<float>(height_) / screen_height;
  
  auto draw_pixel_aa = [&](int x, int y, uint8_t r, uint8_t g, uint8_t b, float coverage) {
      if (x < 0 || x >= static_cast<int>(width_) || 
          y < 0 || y >= static_cast<int>(height_)) return;
      
      size_t idx = (y * width_ + x) * 4;
      
      // Simple alpha blending: src * coverage + dst * (1 - coverage)
      // Assuming line color is opaque (alpha 1.0) scaled by coverage
      float bg_r = pixel_buffer_[idx + 0];
      float bg_g = pixel_buffer_[idx + 1];
      float bg_b = pixel_buffer_[idx + 2];
      
      pixel_buffer_[idx + 0] = static_cast<uint8_t>(r * coverage + bg_r * (1.0f - coverage));
      pixel_buffer_[idx + 1] = static_cast<uint8_t>(g * coverage + bg_g * (1.0f - coverage));
      pixel_buffer_[idx + 2] = static_cast<uint8_t>(b * coverage + bg_b * (1.0f - coverage));
      pixel_buffer_[idx + 3] = 255; 
  };
  
  auto ipart = [](float x) { return std::floor(x); };
  auto round = [](float x) { return std::floor(x + 0.5f); };
  auto fpart = [](float x) { return x - std::floor(x); };
  auto rfpart = [&](float x) { return 1.0f - fpart(x); };

  for (const auto& line : lines) {
    float x0 = line.x1 * scale_x;
    float y0 = line.y1 * scale_y;
    float x1 = line.x2 * scale_x;
    float y1 = line.y2 * scale_y;
    
    uint8_t r = static_cast<uint8_t>(line.r * 255);
    uint8_t g = static_cast<uint8_t>(line.g * 255);
    uint8_t b = static_cast<uint8_t>(line.b * 255);

    bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
    
    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    
    float dx = x1 - x0;
    float dy = y1 - y0;
    float gradient = (dx == 0.0f) ? 1.0f : dy / dx;

    // Handle first endpoint
    float xend = round(x0);
    float yend = y0 + gradient * (xend - x0);
    float xgap = rfpart(x0 + 0.5f);
    int xpxl1 = static_cast<int>(xend);
    int ypxl1 = static_cast<int>(ipart(yend));
    
    if (steep) {
        draw_pixel_aa(ypxl1, xpxl1, r, g, b, rfpart(yend) * xgap);
        draw_pixel_aa(ypxl1 + 1, xpxl1, r, g, b, fpart(yend) * xgap);
    } else {
        draw_pixel_aa(xpxl1, ypxl1, r, g, b, rfpart(yend) * xgap);
        draw_pixel_aa(xpxl1, ypxl1 + 1, r, g, b, fpart(yend) * xgap);
    }
    
    float intery = yend + gradient;
    
    // Handle second endpoint
    xend = round(x1);
    yend = y1 + gradient * (xend - x1);
    xgap = fpart(x1 + 0.5f);
    int xpxl2 = static_cast<int>(xend);
    int ypxl2 = static_cast<int>(ipart(yend));
    
    if (steep) {
        draw_pixel_aa(ypxl2, xpxl2, r, g, b, rfpart(yend) * xgap);
        draw_pixel_aa(ypxl2 + 1, xpxl2, r, g, b, fpart(yend) * xgap);
    } else {
        draw_pixel_aa(xpxl2, ypxl2, r, g, b, rfpart(yend) * xgap);
        draw_pixel_aa(xpxl2, ypxl2 + 1, r, g, b, fpart(yend) * xgap);
    }
    
    // Main loop
    if (steep) {
        for (int x = xpxl1 + 1; x < xpxl2; x++) {
            draw_pixel_aa(static_cast<int>(ipart(intery)), x, r, g, b, rfpart(intery));
            draw_pixel_aa(static_cast<int>(ipart(intery)) + 1, x, r, g, b, fpart(intery));
            intery += gradient;
        }
    } else {
        for (int x = xpxl1 + 1; x < xpxl2; x++) {
            draw_pixel_aa(x, static_cast<int>(ipart(intery)), r, g, b, rfpart(intery));
            draw_pixel_aa(x, static_cast<int>(ipart(intery)) + 1, r, g, b, fpart(intery));
            intery += gradient;
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
