#ifndef MPL_WGPU_EXAMPLE_WINDOW_H_
#define MPL_WGPU_EXAMPLE_WINDOW_H_

#include "../../src/backend/webgpu.hpp"
#include <GLFW/glfw3.h>
#include <string>
#include <functional>
#include <memory>

namespace mpl_wgpu {

class Window {
 public:
  Window(const std::string& title, uint32_t width, uint32_t height);
  ~Window();

  // Initialize WebGPU and Window
  bool Initialize();

  // Run the main loop with a render callback
  void Run(std::function<void(wgpu::RenderPassEncoder&)> render_callback);

  // Accessors
  wgpu::Device GetDevice() const { return device_; }
  wgpu::Queue GetQueue() const { return queue_; }
  wgpu::TextureFormat GetFormat() const { return format_; }
  uint32_t GetWidth() const { return width_; }
  uint32_t GetHeight() const { return height_; }
  GLFWwindow* GetGLFWWindow() const { return window_; }

 private:
  std::string title_;
  uint32_t width_;
  uint32_t height_;
  GLFWwindow* window_ = nullptr;

  // WebGPU objects
  wgpu::Instance instance_;
  wgpu::Surface surface_;
  wgpu::Adapter adapter_;
  wgpu::Device device_;
  wgpu::Queue queue_;
  wgpu::TextureFormat format_ = wgpu::TextureFormat::BGRA8Unorm;

  void InitGLFW();
  void InitWebGPU();
};

}  // namespace mpl_wgpu

#endif  // MPL_WGPU_EXAMPLE_WINDOW_H_
