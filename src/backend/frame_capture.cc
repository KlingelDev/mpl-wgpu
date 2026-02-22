// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

#include "frame_capture.h"
#include <cstring>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "vendor/stb_image_write.h"

namespace mpl_wgpu {

FrameCapture::FrameCapture(wgpu::Device device,
                           uint32_t width, uint32_t height)
    : width_(width),
      height_(height),
      padded_row_(PaddedBytesPerRow(width)) {
  // Offscreen RGBA8Unorm texture (render target + copy source).
  wgpu::TextureDescriptor tex_desc{};
  tex_desc.label = "FrameCaptureTexture";
  tex_desc.usage = static_cast<uint32_t>(
      wgpu::TextureUsage::RenderAttachment
      | wgpu::TextureUsage::CopySrc);
  tex_desc.size.width = width;
  tex_desc.size.height = height;
  tex_desc.size.depthOrArrayLayers = 1;
  tex_desc.format = wgpu::TextureFormat::RGBA8Unorm;
  tex_desc.mipLevelCount = 1;
  tex_desc.sampleCount = 1;
  tex_desc.dimension = static_cast<uint32_t>(
      wgpu::TextureDimension::e2D);
  texture_ = device.CreateTexture(&tex_desc);
  view_ = texture_.CreateView();

  // Staging buffer (map-read + copy-dest).
  wgpu::BufferDescriptor buf_desc{};
  buf_desc.label = "FrameCaptureStagingBuffer";
  buf_desc.size = static_cast<uint64_t>(padded_row_) * height;
  buf_desc.usage = static_cast<uint32_t>(
      wgpu::BufferUsage::MapRead
      | wgpu::BufferUsage::CopyDst);
  staging_buffer_ = device.CreateBuffer(&buf_desc);
}

WGPURenderPassDescriptor FrameCapture::BeginPassDescriptor() {
  color_attachment_ = {};
  color_attachment_.view = view_;
  color_attachment_.loadOp = WGPULoadOp_Clear;
  color_attachment_.storeOp = WGPUStoreOp_Store;
  // Clear to white.
  color_attachment_.clearValue = {1.0, 1.0, 1.0, 1.0};

  WGPURenderPassDescriptor desc{};
  desc.colorAttachmentCount = 1;
  desc.colorAttachments = &color_attachment_;
  return desc;
}

std::vector<uint8_t> FrameCapture::CopyAndReadback(
    wgpu::CommandEncoder& encoder, wgpu::Queue queue,
    wgpu::Device device) {
  // Copy texture to staging buffer.
  WGPUImageCopyTexture src{};
  src.texture = texture_.Get();
  src.mipLevel = 0;
  src.origin = {0, 0, 0};
  src.aspect = WGPUTextureAspect_All;

  WGPUImageCopyBuffer dst{};
  dst.buffer = staging_buffer_.Get();
  dst.layout.offset = 0;
  dst.layout.bytesPerRow = padded_row_;
  dst.layout.rowsPerImage = height_;

  WGPUExtent3D copy_size{width_, height_, 1};
  encoder.CopyTextureToBuffer(&src, &dst, &copy_size);

  // Submit and wait.
  wgpu::CommandBuffer cmd = encoder.Finish();
  queue.Submit(1, &cmd);

  // Map staging buffer synchronously.
  struct MapState {
    bool done = false;
    WGPUBufferMapAsyncStatus status;
  };
  MapState state;
  staging_buffer_.MapAsync(
      WGPUMapMode_Read, 0,
      static_cast<size_t>(padded_row_) * height_,
      [](WGPUBufferMapAsyncStatus status, void* userdata) {
        auto* s = static_cast<MapState*>(userdata);
        s->status = status;
        s->done = true;
      },
      &state);
  device.Poll(true);

  if (state.status != WGPUBufferMapAsyncStatus_Success) {
    throw std::runtime_error(
        "Failed to map staging buffer");
  }

  // Read pixels, strip row padding.
  const uint8_t* mapped =
      static_cast<const uint8_t*>(
          staging_buffer_.GetConstMappedRange(
              0, static_cast<size_t>(padded_row_) * height_));

  uint32_t unpadded_row = width_ * 4;
  std::vector<uint8_t> pixels(
      static_cast<size_t>(unpadded_row) * height_);
  for (uint32_t row = 0; row < height_; ++row) {
    std::memcpy(
        pixels.data() + row * unpadded_row,
        mapped + row * padded_row_,
        unpadded_row);
  }

  staging_buffer_.Unmap();
  return pixels;
}

}  // namespace mpl_wgpu
