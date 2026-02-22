// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

/// SDL2 window with wgpu-native surface for interactive display.

#ifndef MPL_WGPU_EXAMPLE_SDL2_WINDOW_H_
#define MPL_WGPU_EXAMPLE_SDL2_WINDOW_H_

#include "../../src/backend/webgpu.hpp"
#include <SDL2/SDL.h>
#include <cstdint>
#include <string>

namespace mpl_wgpu {

/// SDL2 window with wgpu surface, adapter, device, and queue.
///
/// Unlike Window (GLFW-based), this class does not own the event
/// loop — the caller polls SDL events and calls BeginFrame /
/// EndFrame for presentation.
class Sdl2Window {
 public:
  Sdl2Window(const std::string& title, uint32_t width,
             uint32_t height);
  ~Sdl2Window();

  // Not copyable or movable.
  Sdl2Window(const Sdl2Window&) = delete;
  Sdl2Window& operator=(const Sdl2Window&) = delete;

  /// Initializes SDL2, creates window, wgpu surface/device.
  /// Returns false on failure.
  bool Initialize();

  /// Returns the current surface texture view for rendering.
  /// Returns nullptr if the frame should be skipped.
  WGPUTextureView BeginFrame();

  /// Submits commands and presents the frame.
  void EndFrame(WGPUTextureView view);

  wgpu::Device GetDevice() const { return device_; }
  wgpu::Queue GetQueue() const { return queue_; }
  wgpu::TextureFormat GetSurfaceFormat() const {
    return surface_format_;
  }
  uint32_t GetWidth() const { return width_; }
  uint32_t GetHeight() const { return height_; }

 private:
  std::string title_;
  uint32_t width_;
  uint32_t height_;
  SDL_Window* window_ = nullptr;

  wgpu::Instance instance_;
  wgpu::Surface surface_;
  wgpu::Adapter adapter_;
  wgpu::Device device_;
  wgpu::Queue queue_;
  wgpu::TextureFormat surface_format_ =
      wgpu::TextureFormat::BGRA8Unorm;

  void InitSDL();
  void InitWebGPU();
};

}  // namespace mpl_wgpu

#endif  // MPL_WGPU_EXAMPLE_SDL2_WINDOW_H_
