// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

/// Offscreen render-to-texture and GPU readback for frame capture.

#ifndef MPL_WGPU_FRAME_CAPTURE_H_
#define MPL_WGPU_FRAME_CAPTURE_H_

#include "webgpu.hpp"
#include <cstdint>
#include <vector>

namespace mpl_wgpu {

/// wgpu requires buffer row alignment of 256 bytes.
constexpr uint32_t kCopyBytesPerRowAlignment = 256;

/// Computes the padded bytes-per-row for a given pixel width.
inline uint32_t PaddedBytesPerRow(uint32_t width) {
  uint32_t unpadded = width * 4;
  return (unpadded + kCopyBytesPerRowAlignment - 1)
      / kCopyBytesPerRowAlignment * kCopyBytesPerRowAlignment;
}

/// Offscreen RGBA8Unorm texture with staging buffer for readback.
///
/// Creates a render-target texture (RenderAttachment|CopySrc) and a
/// staging buffer (MapRead|CopyDst) with 256-byte row alignment.
/// After rendering into the texture via BeginPassDescriptor(), call
/// CopyAndReadback() to get tightly-packed RGBA pixels.
class FrameCapture {
 public:
  FrameCapture(wgpu::Device device, uint32_t width,
               uint32_t height);

  /// Returns a render pass descriptor targeting the offscreen
  /// texture, with clear-to-white load op.
  WGPURenderPassDescriptor BeginPassDescriptor();

  /// Copies texture to staging buffer, maps, strips padding, and
  /// returns tightly-packed RGBA pixel data.
  std::vector<uint8_t> CopyAndReadback(
      wgpu::CommandEncoder& encoder, wgpu::Queue queue,
      wgpu::Device device);

  uint32_t width() const { return width_; }
  uint32_t height() const { return height_; }

 private:
  wgpu::Texture texture_;
  WGPUTextureView view_;
  wgpu::Buffer staging_buffer_;
  uint32_t width_;
  uint32_t height_;
  uint32_t padded_row_;

  WGPURenderPassColorAttachment color_attachment_;
};

}  // namespace mpl_wgpu

#endif  // MPL_WGPU_FRAME_CAPTURE_H_
