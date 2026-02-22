// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

#include "sdl2_window.h"
#include <iostream>
#include <stdexcept>

#ifdef __linux__
#include <SDL2/SDL_syswm.h>
#endif

namespace mpl_wgpu {

namespace {

wgpu::Surface CreateSurfaceForSDLWindow(
    wgpu::Instance instance, SDL_Window* window) {
#ifdef __linux__
  SDL_SysWMinfo wm_info;
  SDL_VERSION(&wm_info.version);
  if (!SDL_GetWindowWMInfo(window, &wm_info)) {
    throw std::runtime_error(
        std::string("SDL_GetWindowWMInfo failed: ")
        + SDL_GetError());
  }

  switch (wm_info.subsystem) {
    case SDL_SYSWM_X11: {
      WGPUSurfaceDescriptorFromXlibWindow x11_desc{};
      x11_desc.chain.sType =
          WGPUSType_SurfaceDescriptorFromXlibWindow;
      x11_desc.display = wm_info.info.x11.display;
      x11_desc.window = wm_info.info.x11.window;

      WGPUSurfaceDescriptor desc{};
      desc.nextInChain =
          reinterpret_cast<const WGPUChainedStruct*>(
              &x11_desc);
      std::cout << "Creating X11 surface" << std::endl;
      return instance.CreateSurface(&desc);
    }
    case SDL_SYSWM_WAYLAND: {
      WGPUSurfaceDescriptorFromWaylandSurface wl_desc{};
      wl_desc.chain.sType =
          WGPUSType_SurfaceDescriptorFromWaylandSurface;
      wl_desc.display = wm_info.info.wl.display;
      wl_desc.surface = wm_info.info.wl.surface;

      WGPUSurfaceDescriptor desc{};
      desc.nextInChain =
          reinterpret_cast<const WGPUChainedStruct*>(
              &wl_desc);
      std::cout << "Creating Wayland surface" << std::endl;
      return instance.CreateSurface(&desc);
    }
    default:
      throw std::runtime_error(
          "Unsupported SDL windowing subsystem");
  }
#else
  throw std::runtime_error(
      "Unsupported platform for Surface creation");
#endif
}

}  // namespace

Sdl2Window::Sdl2Window(const std::string& title,
                       uint32_t width, uint32_t height)
    : title_(title), width_(width), height_(height) {}

Sdl2Window::~Sdl2Window() {
  if (window_) {
    SDL_DestroyWindow(window_);
  }
  SDL_Quit();
}

bool Sdl2Window::Initialize() {
  try {
    InitSDL();
    InitWebGPU();
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Initialization failed: " << e.what()
              << std::endl;
    return false;
  }
}

void Sdl2Window::InitSDL() {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    throw std::runtime_error(
        std::string("SDL_Init failed: ") + SDL_GetError());
  }

  window_ = SDL_CreateWindow(
      title_.c_str(),
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      static_cast<int>(width_),
      static_cast<int>(height_),
      SDL_WINDOW_SHOWN);
  if (!window_) {
    throw std::runtime_error(
        std::string("SDL_CreateWindow failed: ")
        + SDL_GetError());
  }
}

void Sdl2Window::InitWebGPU() {
  WGPUInstanceDescriptor instance_desc{};
  instance_ = wgpu::CreateInstance(&instance_desc);
  if (!instance_) {
    throw std::runtime_error(
        "Failed to create WebGPU Instance");
  }

  surface_ = CreateSurfaceForSDLWindow(instance_, window_);
  if (!surface_) {
    throw std::runtime_error(
        "Failed to create WebGPU Surface");
  }

  WGPURequestAdapterOptions adapter_opts{};
  adapter_opts.compatibleSurface = surface_.Get();
  adapter_ = instance_.RequestAdapter(&adapter_opts);
  if (!adapter_) {
    throw std::runtime_error(
        "Failed to request WebGPU Adapter");
  }

  WGPUDeviceDescriptor device_desc{};
  device_desc.label = "mpl-wgpu SDL2 Device";
  device_ = adapter_.RequestDevice(&device_desc);
  if (!device_) {
    throw std::runtime_error(
        "Failed to request WebGPU Device");
  }

  queue_ = device_.GetQueue();

  // Configure surface.
  WGPUSurfaceConfiguration config{};
  config.device = device_.Get();
  config.format =
      static_cast<WGPUTextureFormat>(surface_format_);
  config.usage = WGPUTextureUsage_RenderAttachment;
  config.alphaMode = WGPUCompositeAlphaMode_Auto;
  config.width = width_;
  config.height = height_;
  config.presentMode = WGPUPresentMode_Fifo;
  surface_.Configure(&config);
}

WGPUTextureView Sdl2Window::BeginFrame() {
  WGPUSurfaceTexture surface_tex;
  surface_.GetCurrentTexture(&surface_tex);

  if (surface_tex.status
      != WGPUSurfaceGetCurrentTextureStatus_Success) {
    return nullptr;
  }

  return wgpuTextureCreateView(surface_tex.texture, nullptr);
}

void Sdl2Window::EndFrame(WGPUTextureView view) {
  surface_.Present();
  wgpuTextureViewRelease(view);
}

}  // namespace mpl_wgpu
