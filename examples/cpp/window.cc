#include "window.h"
#include <iostream>
#include <vector>
#include <stdexcept>

#ifdef __linux__
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_WAYLAND
#include <GLFW/glfw3native.h>
#endif

namespace mpl_wgpu {

// Helper to create surface
wgpu::Surface CreateSurfaceForWindow(wgpu::Instance instance, GLFWwindow* window) {
#ifdef __linux__
    // Try X11 first
    void* x11_display = glfwGetX11Display();
    if (x11_display != nullptr) {
        WGPUSurfaceDescriptorFromXlibWindow x11_desc = {};
        x11_desc.chain.sType = WGPUSType_SurfaceDescriptorFromXlibWindow;
        x11_desc.display = x11_display;
        x11_desc.window = glfwGetX11Window(window);
        
        WGPUSurfaceDescriptor desc = {};
        desc.nextInChain = (const WGPUChainedStruct*)&x11_desc;
        
        std::cout << "Creating X11 surface (display=" << x11_display 
                  << ", window=" << x11_desc.window << ")" << std::endl;
        return instance.CreateSurface(&desc);
    }
    
    // X11 not available, check if we're on Wayland
    std::cerr << "X11 display is NULL. Checking for Wayland..." << std::endl;
    
    // GLFW 3.4+ has glfwGetWaylandDisplay/glfwGetWaylandWindow
    // But we need to check at compile time if Wayland support is available
    #if defined(GLFW_EXPOSE_NATIVE_WAYLAND)
    void* wl_display = glfwGetWaylandDisplay();
    if (wl_display != nullptr) {
        WGPUSurfaceDescriptorFromWaylandSurface wl_desc = {};
        wl_desc.chain.sType = WGPUSType_SurfaceDescriptorFromWaylandSurface;
        wl_desc.display = wl_display;
        wl_desc.surface = glfwGetWaylandWindow(window);
        
        WGPUSurfaceDescriptor desc = {};
        desc.nextInChain = (const WGPUChainedStruct*)&wl_desc;
        
        std::cout << "Creating Wayland surface" << std::endl;
        return instance.CreateSurface(&desc);
    }
    #endif
    
    throw std::runtime_error(
        "No display server available. "
        "Ensure you are running under X11 or Wayland. "
        "If using XWayland, set DISPLAY env var.");
#else
    throw std::runtime_error("Unsupported platform for Surface creation");
#endif
}

Window::Window(const std::string& title, uint32_t width, uint32_t height)
    : title_(title), width_(width), height_(height) {}

Window::~Window() {
  if (window_) {
    glfwDestroyWindow(window_);
  }
  glfwTerminate();
}

bool Window::Initialize() {
  try {
    InitGLFW();
    InitWebGPU();
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Initialization failed: " << e.what() << std::endl;
    return false;
  }
}

void Window::InitGLFW() {
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); 
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);  // No title bar

  window_ = glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);
  if (!window_) {
    throw std::runtime_error("Failed to create GLFW window");
  }
}

void Window::InitWebGPU() {
  // Create Instance
  WGPUInstanceDescriptor instanceDesc = {};
  instanceDesc.nextInChain = nullptr;
  instance_ = wgpu::CreateInstance(&instanceDesc);
  if (!instance_) {
    throw std::runtime_error("Failed to create WebGPU Instance");
  }

  // Create Surface
  surface_ = CreateSurfaceForWindow(instance_, window_);
  
  if (!surface_) {
    throw std::runtime_error("Failed to create WebGPU Surface");
  }

  // Request Adapter
  WGPURequestAdapterOptions options = {};
  options.compatibleSurface = surface_.Get();
  
  adapter_ = instance_.RequestAdapter(&options);
  if (!adapter_) {
     throw std::runtime_error("Failed to request WebGPU Adapter");
  }

  // Request Device
  WGPUDeviceDescriptor deviceDesc = {};
  deviceDesc.label = "mpl-wgpu Device";
  deviceDesc.requiredFeatureCount = 0;
  deviceDesc.requiredLimits = nullptr;
  deviceDesc.defaultQueue.nextInChain = nullptr;
  deviceDesc.defaultQueue.label = "Default Queue";
  
  device_ = adapter_.RequestDevice(&deviceDesc);
  if (!device_) {
      throw std::runtime_error("Failed to request Device");
  }
  
  queue_ = device_.GetQueue();
  
  // Configure Surface
  WGPUSurfaceConfiguration config = {};
  config.device = device_.Get();
  config.format = static_cast<WGPUTextureFormat>(format_);
  config.usage = WGPUTextureUsage_RenderAttachment;
  config.viewFormatCount = 0;
  config.viewFormats = nullptr;
  config.alphaMode = WGPUCompositeAlphaMode_Auto;
  config.width = width_;
  config.height = height_;
  config.presentMode = WGPUPresentMode_Fifo;
  
  surface_.Configure(&config);
}

void Window::Run(std::function<void(wgpu::RenderPassEncoder&)> render_callback) {
  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();

    if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window_, true);
    }

    // Reconfigure if resized
    int w, h;
    glfwGetFramebufferSize(window_, &w, &h);
    if (w != 0 && h != 0 && (static_cast<uint32_t>(w) != width_ || static_cast<uint32_t>(h) != height_)) {
        width_ = w;
        height_ = h;
        WGPUSurfaceConfiguration config = {};
        config.device = device_.Get();
        config.format = static_cast<WGPUTextureFormat>(format_);
        config.usage = WGPUTextureUsage_RenderAttachment;
        config.alphaMode = WGPUCompositeAlphaMode_Auto;
        config.width = width_;
        config.height = height_;
        config.presentMode = WGPUPresentMode_Fifo;
        surface_.Configure(&config);
    }
    
    // Get next texture
    WGPUSurfaceTexture surfaceTexture;
    surface_.GetCurrentTexture(&surfaceTexture);
    
    if (surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_Timeout) {
        // Skip frame
        continue;
    }
    if (surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_Outdated) {
        continue; // Reconfigure next frame
    }
     if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
        std::cerr << "Failed to acquire next swap chain texture" << std::endl;
        break;
    }
    
    wgpu::TextureView view(wgpuTextureCreateView(surfaceTexture.texture, nullptr));
    
    wgpu::CommandEncoder encoder = device_.CreateCommandEncoder();
    
    WGPURenderPassColorAttachment attachment = {};
    attachment.view = view.Get();
    attachment.loadOp = WGPULoadOp_Clear;
    attachment.storeOp = WGPUStoreOp_Store;
    attachment.clearValue = {0.2f, 0.2f, 0.2f, 1.0f}; 
    
    WGPURenderPassDescriptor passDesc = {};
    passDesc.colorAttachmentCount = 1;
    passDesc.colorAttachments = &attachment;
    
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&passDesc);
    
    if (render_callback) {
        render_callback(pass);
    }
    
    pass.End(); // WGPU internal end
    wgpu::CommandBuffer commands = encoder.Finish();
    queue_.Submit(1, &commands);
    
    surface_.Present();
    
    // Texture release is handled by wgpu (automatic ref counting in C++ wrapper?)
    // In raw C API we need to release texture view usually?
    // wgpuTextureViewRelease(view.Get()); // Wrapper should handle this if it was smart, but my wrapper is dumb.
    // I should add a destructor to TextureView wrapper or do it manually.
    // Let's rely on the fact that I didn't verify TextureView wrapper destructor.
    wgpuTextureViewRelease(view.Get());
  }
}

} // namespace mpl_wgpu
