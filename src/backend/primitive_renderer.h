// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

/// GPU-accelerated primitive renderer
/// Handles rectangles, lines, circles, triangles with batching

#ifndef MPL_WGPU_PRIMITIVE_RENDERER_H_
#define MPL_WGPU_PRIMITIVE_RENDERER_H_

#include "webgpu.hpp"
#include <array>
#include <vector>

namespace mpl_wgpu {

/// Instance data for GPU rendering (16-byte aligned)
struct Instance {
  float pos_a_radius[4];   // pos_a (x,y,z) + radius
  float pos_b_width[4];    // pos_b (x,y,z) + stroke_width
  float color[4];          // RGBA color
  float params[4];         // prim_type, dash_len, gap_len, offset
  float pos_c_pad[4];      // pos_c or padding
};

static_assert(sizeof(Instance) == 80, "Instance must be 80 bytes");

/// Primitive types
enum class PrimitiveType : uint32_t {
  kRectangle = 0,
  kCircle = 1,
  kLine = 2,
  kTriangle = 30,
  kTriangleUnlit = 31,
};

/// GPU primitive renderer using batched instancing
class PrimitiveRenderer {
 public:
  PrimitiveRenderer(wgpu::Device device, wgpu::TextureFormat format,
                    uint32_t width, uint32_t height);
  ~PrimitiveRenderer() = default;

  // Resize handling
  void Resize(uint32_t width, uint32_t height);

  // Drawing methods
  void DrawRect(float x, float y, float width, float height,
                const std::array<float, 4>& color, float radius = 0.0f,
                float stroke_width = 0.0f);

  void DrawLine(float x1, float y1, float z1, float x2, float y2,
                float z2, float thickness,
                const std::array<float, 4>& color,
                float dash_len = 0.0f, float gap_len = 0.0f,
                float dash_offset = 0.0f);

  void DrawCircle(float cx, float cy, float cz, float radius,
                  const std::array<float, 4>& color,
                  float stroke_width = 0.0f);

  void DrawTriangle(float x1, float y1, float z1, float x2, float y2,
                    float z2, float x3, float y3, float z3,
                    const std::array<float, 4>& color,
                    bool lit = true);

  // Prepare for rendering (upload to GPU)
  void Prepare(wgpu::Queue queue);

  // Render all primitives
  void Render(wgpu::RenderPassEncoder& pass);

  // Clear batched primitives
  void Clear();

 private:
  wgpu::Device device_;
  wgpu::RenderPipeline pipeline_depth_write_;
  wgpu::RenderPipeline pipeline_lines_;
  wgpu::BindGroup bind_group_;
  wgpu::Buffer uniform_buffer_;
  wgpu::Buffer instance_buffer_;

  std::vector<Instance> instances_;
  uint32_t width_;
  uint32_t height_;
  size_t capacity_;

  void UpdateUniforms(wgpu::Queue queue);
  void ResizeInstanceBuffer(size_t new_capacity);
};

}  // namespace mpl_wgpu

#endif  // MPL_WGPU_PRIMITIVE_RENDERER_H_