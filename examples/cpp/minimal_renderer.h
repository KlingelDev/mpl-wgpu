// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

/// Minimal WgpuRenderer implementation for examples
/// Connects to actual wgpu device and primitives

#ifndef MPL_WGPU_EXAMPLES_MINIMAL_RENDERER_H_
#define MPL_WGPU_EXAMPLES_MINIMAL_RENDERER_H_

#include <matplot/backend/wgpu_backend.h>
#include <array>
#include <memory>
#include <vector>

namespace mpl_wgpu {

/// Minimal renderer using wgpu-native
/// TODO: Replace with actual wgpu integration
class MinimalRenderer : public matplot::backend::WgpuRenderer {
 public:
  MinimalRenderer(unsigned int width, unsigned int height);
  ~MinimalRenderer() override = default;

  void DrawRects(const std::vector<Rect>& rects, float screen_width,
                 float screen_height) override;

  void DrawLines(const std::vector<Line>& lines, float screen_width,
                 float screen_height) override;

  void DrawCircles(const std::vector<Circle>& circles,
                   float screen_width, float screen_height) override;

  void DrawTriangles(const std::vector<Triangle>& triangles,
                     float screen_width, float screen_height) override;

  void DrawText(const std::string& text, float x, float y,
                float font_size, const std::array<float, 4>& color,
                float rotation = 0.0f) override;

  void Clear(const std::array<float, 4>& color) override;

  void DrawImage(const std::vector<float>& data, size_t img_width,
                 size_t img_height, float x, float y, float width,
                 float height) override;

  // Get pixel buffer for display
  const uint8_t* GetPixels() const;
  size_t GetPixelCount() const;

 private:
  unsigned int width_;
  unsigned int height_;
  std::vector<uint8_t> pixel_buffer_;

  // TODO: Add wgpu device, queue, render pass
};

}  // namespace mpl_wgpu

#endif  // MPL_WGPU_EXAMPLES_MINIMAL_RENDERER_H_
