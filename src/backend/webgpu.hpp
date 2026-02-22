// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

/// Minimal C++ wrapper for WebGPU C API
/// Provides wgpu:: namespace for ergonomic usage

#ifndef MPL_WGPU_WEBGPU_HPP_
#define MPL_WGPU_WEBGPU_HPP_

#include <webgpu.h>
#include <wgpu.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace wgpu {

// Forward declarations
class Device;
class Queue;
class Buffer;
class Texture;
class TextureView;
class ShaderModule;
class RenderPipeline;
class BindGroup;
class BindGroupLayout;
class PipelineLayout;
class RenderPassEncoder;
class CommandEncoder;
class CommandBuffer;
class Instance;
class Adapter;
class Surface;
struct ImageCopyTextureWgpu;
struct TextureDataLayoutWgpu;

// Enums (keep as is)
enum class TextureFormat : uint32_t {
  BGRA8Unorm = WGPUTextureFormat_BGRA8Unorm,
  RGBA8Unorm = WGPUTextureFormat_RGBA8Unorm,
  R8Unorm = WGPUTextureFormat_R8Unorm,
};

enum class TextureDimension : uint32_t {
  e1D = WGPUTextureDimension_1D,
  e2D = WGPUTextureDimension_2D,
  e3D = WGPUTextureDimension_3D,
};

enum class BufferUsage : uint32_t {
  Uniform = WGPUBufferUsage_Uniform,
  Vertex = WGPUBufferUsage_Vertex,
  CopyDst = WGPUBufferUsage_CopyDst,
  MapRead = WGPUBufferUsage_MapRead,
  CopySrc = WGPUBufferUsage_CopySrc,
};

inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
  return static_cast<BufferUsage>(
      static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

enum class ShaderStage : uint32_t {
  Vertex = WGPUShaderStage_Vertex,
  Fragment = WGPUShaderStage_Fragment,
};

inline ShaderStage operator|(ShaderStage a, ShaderStage b) {
  return static_cast<ShaderStage>(
      static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

enum class VertexFormat : uint32_t {
  Float32x4 = WGPUVertexFormat_Float32x4,
};

enum class VertexStepMode : uint32_t {
  Instance = WGPUVertexStepMode_Instance,
};

enum class PrimitiveTopology : uint32_t {
  TriangleList = WGPUPrimitiveTopology_TriangleList,
};

enum class CullMode : uint32_t {
  None = WGPUCullMode_None,
};

enum class BlendOperation : uint32_t {
  Add = WGPUBlendOperation_Add,
};

enum class BlendFactor : uint32_t {
  SrcAlpha = WGPUBlendFactor_SrcAlpha,
  OneMinusSrcAlpha = WGPUBlendFactor_OneMinusSrcAlpha,
  One = WGPUBlendFactor_One,
};

enum class ColorWriteMask : uint32_t {
  All = WGPUColorWriteMask_All,
};

enum class BufferBindingType : uint32_t {
  Uniform = WGPUBufferBindingType_Uniform,
};

enum class SamplerBindingType : uint32_t {
  Filtering = WGPUSamplerBindingType_Filtering,
};

enum class TextureSampleType : uint32_t {
  Float = WGPUTextureSampleType_Float,
};

enum class TextureViewDimension : uint32_t {
  e2D = WGPUTextureViewDimension_2D,
};

enum class TextureUsage : uint32_t {
  CopyDst = WGPUTextureUsage_CopyDst,
  CopySrc = WGPUTextureUsage_CopySrc,
  TextureBinding = WGPUTextureUsage_TextureBinding,
  RenderAttachment = WGPUTextureUsage_RenderAttachment,
};

inline TextureUsage operator|(TextureUsage a, TextureUsage b) {
  return static_cast<TextureUsage>(
      static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

enum class AddressMode : uint32_t {
  ClampToEdge = WGPUAddressMode_ClampToEdge,
};

enum class FilterMode : uint32_t {
  Linear = WGPUFilterMode_Linear,
};

// Descriptors
struct ShaderModuleWGSLDescriptor {
  const char* code;
};

struct ShaderModuleDescriptor {
  ShaderModuleWGSLDescriptor* nextInChain;
};

struct BufferDescriptor {
  const char* label;
  uint64_t size;
  uint32_t usage;
};

struct TextureDescriptor {
  const char* label;
  uint32_t usage; 
  struct {
     uint32_t width;
     uint32_t height;
     uint32_t depthOrArrayLayers;
  } size;
  TextureFormat format;
  uint32_t mipLevelCount;
  uint32_t sampleCount;
  uint32_t dimension;
};

struct SamplerDescriptor {
  const char* label;
  AddressMode addressModeU;
  AddressMode addressModeV;
  AddressMode addressModeW;
  FilterMode magFilter;
  FilterMode minFilter;
  FilterMode mipmapFilter;
  uint16_t maxAnisotropy;
};

struct BindGroupLayoutEntry {
  uint32_t binding;
  uint32_t visibility;
  struct {
    BufferBindingType type;
  } buffer;
  struct {
    TextureSampleType sampleType;
    TextureViewDimension viewDimension;
  } texture;
  struct {
     SamplerBindingType type;
  } sampler;
};

struct BindGroupLayoutDescriptor {
  uint32_t entryCount;
  const BindGroupLayoutEntry* entries;
};

struct BindGroupEntry {
  uint32_t binding;
  WGPUBuffer buffer;
  uint64_t size;
  WGPUTextureView textureView;
  WGPUSampler sampler;
};

struct BindGroupDescriptor {
  WGPUBindGroupLayout layout;
  uint32_t entryCount;
  const BindGroupEntry* entries;
};

struct PipelineLayoutDescriptor {
  uint32_t bindGroupLayoutCount;
  const WGPUBindGroupLayout* bindGroupLayouts;
};

struct VertexAttribute {
  VertexFormat format;
  uint64_t offset;
  uint32_t shaderLocation;
};

struct VertexBufferLayout {
  uint64_t arrayStride;
  VertexStepMode stepMode;
  size_t attributeCount;  // Must be size_t to match WGPUVertexBufferLayout
  const VertexAttribute* attributes;
};

struct BlendComponent {
  BlendOperation operation;
  BlendFactor srcFactor;
  BlendFactor dstFactor;
};

struct BlendState {
  BlendComponent color;
  BlendComponent alpha;
};

struct ColorTargetState {
  TextureFormat format;
  BlendState* blend;
  ColorWriteMask writeMask;
};

struct FragmentState {
  WGPUShaderModule module;
  const char* entryPoint;
  uint32_t targetCount;
  const ColorTargetState* targets;
};

struct VertexState {
  WGPUShaderModule module;
  const char* entryPoint;
  uint32_t bufferCount;
  const VertexBufferLayout* buffers;
};

struct PrimitiveState {
  PrimitiveTopology topology;
  CullMode cullMode;
};

struct MultisampleState {
  uint32_t count;
};


struct RenderPipelineDescriptor {
  WGPUPipelineLayout layout;
  VertexState vertex;
  const FragmentState* fragment;
  PrimitiveState primitive;
  MultisampleState multisample;
};

// --- CLASS DECLARATIONS ---

class Buffer {
 public:
  Buffer() : handle_(nullptr) {}
  explicit Buffer(WGPUBuffer h) : handle_(h) {}
  WGPUBuffer Get() const { return handle_; }
  uint64_t GetSize() const { return 0; }  // TODO

  void MapAsync(WGPUMapModeFlags mode, size_t offset,
                size_t size,
                WGPUBufferMapCallback callback,
                void* userdata) const {
    wgpuBufferMapAsync(handle_, mode, offset, size,
                       callback, userdata);
  }

  const void* GetConstMappedRange(size_t offset,
                                  size_t size) const {
    return wgpuBufferGetConstMappedRange(
        handle_, offset, size);
  }

  void Unmap() const { wgpuBufferUnmap(handle_); }

 private:
  WGPUBuffer handle_;
};

class Texture {
 public:
  Texture() : handle_(nullptr) {}
  explicit Texture(WGPUTexture h) : handle_(h) {}
  WGPUTexture Get() const { return handle_; }
  WGPUTextureView CreateView() const { return wgpuTextureCreateView(handle_, nullptr); }
 private:
  WGPUTexture handle_;
};

class Sampler {
 public:
  Sampler() : handle_(nullptr) {}
  explicit Sampler(WGPUSampler h) : handle_(h) {}
  WGPUSampler Get() const { return handle_; }
 private:
  WGPUSampler handle_;
};


class TextureView {
 public:
  TextureView() : handle_(nullptr) {}
  explicit TextureView(WGPUTextureView h) : handle_(h) {}
  WGPUTextureView Get() const { return handle_; }
 private:
  WGPUTextureView handle_;
};

class ShaderModule {
 public:
  ShaderModule() : handle_(nullptr) {}
  explicit ShaderModule(WGPUShaderModule h) : handle_(h) {}
  WGPUShaderModule Get() const { return handle_; }
 private:
  WGPUShaderModule handle_;
};

class BindGroupLayout {
 public:
  BindGroupLayout() : handle_(nullptr) {}
  explicit BindGroupLayout(WGPUBindGroupLayout h) : handle_(h) {}
  WGPUBindGroupLayout Get() const { return handle_; }
 private:
  WGPUBindGroupLayout handle_;
};

class BindGroup {
 public:
  BindGroup() : handle_(nullptr) {}
  explicit BindGroup(WGPUBindGroup h) : handle_(h) {}
  WGPUBindGroup Get() const { return handle_; }
 private:
  WGPUBindGroup handle_;
};

class PipelineLayout {
 public:
  PipelineLayout() : handle_(nullptr) {}
  explicit PipelineLayout(WGPUPipelineLayout h) : handle_(h) {}
  WGPUPipelineLayout Get() const { return handle_; }
 private:
  WGPUPipelineLayout handle_;
};

class RenderPipeline {
 public:
  RenderPipeline() : handle_(nullptr) {}
  explicit RenderPipeline(WGPURenderPipeline h) : handle_(h) {}
  WGPURenderPipeline Get() const { return handle_; }
 private:
  WGPURenderPipeline handle_;
};

class CommandBuffer {
 public:
  CommandBuffer() : handle_(nullptr) {}
  explicit CommandBuffer(WGPUCommandBuffer h) : handle_(h) {}
  WGPUCommandBuffer Get() const { return handle_; }
 private:
  WGPUCommandBuffer handle_;
};

class Surface {
 public:
  Surface() : handle_(nullptr) {}
  explicit Surface(WGPUSurface h) : handle_(h) {}
  operator bool() const { return handle_ != nullptr; }
  
  WGPUSurface Get() const { return handle_; }
  
  void Configure(const WGPUSurfaceConfiguration* config) const;
  void GetCurrentTexture(WGPUSurfaceTexture* texture) const;
  void Present() const;
  
 private:
  WGPUSurface handle_;
};

class RenderPassEncoder {
 public:
  RenderPassEncoder() : handle_(nullptr) {}
  explicit RenderPassEncoder(WGPURenderPassEncoder h) : handle_(h) {}
  
  void SetBindGroup(uint32_t index, BindGroup group, const uint32_t* offsets) const;
  void SetBindGroup(uint32_t index, BindGroup group) const;
  void SetVertexBuffer(uint32_t slot, Buffer buffer) const;
  void SetPipeline(RenderPipeline pipeline) const;
  void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const;
  
  void End() const { wgpuRenderPassEncoderEnd(handle_); }
  
 private:
  WGPURenderPassEncoder handle_;
};

class CommandEncoder {
 public:
  CommandEncoder() : handle_(nullptr) {}
  explicit CommandEncoder(WGPUCommandEncoder h)
      : handle_(h) {}

  RenderPassEncoder BeginRenderPass(
      const WGPURenderPassDescriptor* desc) const;
  CommandBuffer Finish(
      const WGPUCommandBufferDescriptor* desc = nullptr)
      const;

  void CopyTextureToBuffer(
      const WGPUImageCopyTexture* source,
      const WGPUImageCopyBuffer* destination,
      const WGPUExtent3D* copy_size) const {
    wgpuCommandEncoderCopyTextureToBuffer(
        handle_, source, destination, copy_size);
  }

 private:
  WGPUCommandEncoder handle_;
};

class Queue {
 public:
  Queue() : handle_(nullptr) {}
  explicit Queue(WGPUQueue h) : handle_(h) {}
  
  void WriteBuffer(Buffer buffer, uint64_t offset, const void* data, size_t size) const;
  void WriteTexture(const ImageCopyTextureWgpu* destination, const void* data, size_t dataSize, const TextureDataLayoutWgpu* dataLayout, const void* writeSize) const; // writeSize is Extent3D
  void Submit(uint32_t commandCount, const CommandBuffer* commands) const;
  
 private:
  WGPUQueue handle_;
};

class Device {
 public:
  Device() : handle_(nullptr) {}
  explicit Device(WGPUDevice h) : handle_(h) {}
  operator bool() const { return handle_ != nullptr; }
  
  WGPUDevice Get() const { return handle_; }
  
  ShaderModule CreateShaderModule(const ShaderModuleDescriptor* desc) const;
  Buffer CreateBuffer(const BufferDescriptor* desc) const;
  Texture CreateTexture(const TextureDescriptor* desc) const;
  Sampler CreateSampler(const SamplerDescriptor* desc) const;
  BindGroupLayout CreateBindGroupLayout(const BindGroupLayoutDescriptor* desc) const;
  BindGroup CreateBindGroup(const BindGroupDescriptor* desc) const;
  PipelineLayout CreatePipelineLayout(const PipelineLayoutDescriptor* desc) const;
  RenderPipeline CreateRenderPipeline(const RenderPipelineDescriptor* desc) const;
  CommandEncoder CreateCommandEncoder(
      const WGPUCommandEncoderDescriptor* desc = nullptr)
      const;
  Queue GetQueue() const;

  void Poll(bool wait) const {
    wgpuDevicePoll(handle_, wait, nullptr);
  }

 private:
  WGPUDevice handle_;
};

class Adapter {
 public:
  Adapter() : handle_(nullptr) {}
  explicit Adapter(WGPUAdapter h) : handle_(h) {}
  operator bool() const { return handle_ != nullptr; }
  
  WGPUAdapter Get() const { return handle_; }
  
  Device RequestDevice(const WGPUDeviceDescriptor* desc) const;
  
 private:
  WGPUAdapter handle_;
};

class Instance {
 public:
  Instance() : handle_(nullptr) {}
  explicit Instance(WGPUInstance h) : handle_(h) {}
  operator bool() const { return handle_ != nullptr; }
  
  WGPUInstance Get() const { return handle_; }
  
  Surface CreateSurface(const WGPUSurfaceDescriptor* desc) const;
  Adapter RequestAdapter(const WGPURequestAdapterOptions* options) const;
  
 private:
  WGPUInstance handle_;
};

struct ImageCopyTextureWgpu {
  Texture texture;
  uint32_t mipLevel;
  struct { uint32_t x; uint32_t y; uint32_t z; } origin;
  // aspect default
};

struct TextureDataLayoutWgpu {
  uint64_t offset;
  uint32_t bytesPerRow;
  uint32_t rowsPerImage;
};

// --- IMPLEMENTATIONS ---

// Global
inline Instance CreateInstance(const WGPUInstanceDescriptor* desc) {
    return Instance(wgpuCreateInstance(desc));
}

// Surface
inline void Surface::Configure(const WGPUSurfaceConfiguration* config) const {
    wgpuSurfaceConfigure(handle_, config);
}
inline void Surface::GetCurrentTexture(WGPUSurfaceTexture* texture) const {
    wgpuSurfaceGetCurrentTexture(handle_, texture);
}
inline void Surface::Present() const {
    wgpuSurfacePresent(handle_);
}

// RenderPassEncoder
inline void RenderPassEncoder::SetBindGroup(uint32_t index, BindGroup group, const uint32_t* offsets) const {
    wgpuRenderPassEncoderSetBindGroup(handle_, index, group.Get(), 0, offsets);
}
inline void RenderPassEncoder::SetBindGroup(uint32_t index, BindGroup group) const {
    wgpuRenderPassEncoderSetBindGroup(handle_, index, group.Get(), 0, nullptr);
}
inline void RenderPassEncoder::SetVertexBuffer(uint32_t slot, Buffer buffer) const {
    // Use WGPU_WHOLE_SIZE to bind the entire buffer
    wgpuRenderPassEncoderSetVertexBuffer(handle_, slot, buffer.Get(), 0, WGPU_WHOLE_SIZE);
}
inline void RenderPassEncoder::SetPipeline(RenderPipeline pipeline) const {
    wgpuRenderPassEncoderSetPipeline(handle_, pipeline.Get());
}
inline void RenderPassEncoder::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const {
    wgpuRenderPassEncoderDraw(handle_, vertexCount, instanceCount, firstVertex, firstInstance);
}

// CommandEncoder
inline RenderPassEncoder CommandEncoder::BeginRenderPass(const WGPURenderPassDescriptor* desc) const {
    return RenderPassEncoder(wgpuCommandEncoderBeginRenderPass(handle_, desc));
}
inline CommandBuffer CommandEncoder::Finish(const WGPUCommandBufferDescriptor* desc) const {
    return CommandBuffer(wgpuCommandEncoderFinish(handle_, desc));
}

// Queue
inline void Queue::WriteBuffer(Buffer buffer, uint64_t offset, const void* data, size_t size) const {
    wgpuQueueWriteBuffer(handle_, buffer.Get(), offset, data, size);
}
inline void Queue::WriteTexture(const ImageCopyTextureWgpu* destination, const void* data, size_t dataSize, const TextureDataLayoutWgpu* dataLayout, const void* writeSize) const {
    WGPUImageCopyTexture dest = {};
    dest.texture = destination->texture.Get();
    dest.mipLevel = destination->mipLevel;
    dest.origin = {destination->origin.x, destination->origin.y, destination->origin.z};
    dest.aspect = WGPUTextureAspect_All;

    WGPUTextureDataLayout layout = {};
    layout.offset = dataLayout->offset;
    layout.bytesPerRow = dataLayout->bytesPerRow;
    layout.rowsPerImage = dataLayout->rowsPerImage;
    
    const WGPUExtent3D* extent = static_cast<const WGPUExtent3D*>(writeSize);
    wgpuQueueWriteTexture(handle_, &dest, data, dataSize, &layout, extent);
}

inline void Queue::Submit(uint32_t commandCount, const CommandBuffer* commands) const {
    std::vector<WGPUCommandBuffer> raw_cmds(commandCount);
    for(uint32_t i=0; i<commandCount; ++i) raw_cmds[i] = commands[i].Get();
    wgpuQueueSubmit(handle_, commandCount, raw_cmds.data());
}

// Device
inline ShaderModule Device::CreateShaderModule(const ShaderModuleDescriptor* desc) const {
  WGPUShaderModuleWGSLDescriptor wgsl = {};
  wgsl.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
  wgsl.code = desc->nextInChain->code;
  
  WGPUShaderModuleDescriptor wgpu_desc = {};
  wgpu_desc.nextInChain = (const WGPUChainedStruct*)&wgsl;
  
  return ShaderModule(wgpuDeviceCreateShaderModule(handle_, &wgpu_desc));
}

inline Buffer Device::CreateBuffer(const BufferDescriptor* desc) const {
  WGPUBufferDescriptor wgpu_desc = {};
  wgpu_desc.label = desc->label;
  wgpu_desc.size = desc->size;
  wgpu_desc.usage = desc->usage;
  return Buffer(wgpuDeviceCreateBuffer(handle_, &wgpu_desc));
}

inline Texture Device::CreateTexture(const TextureDescriptor* desc) const {
    WGPUTextureDescriptor d{};
    d.label = desc->label;
    d.usage = desc->usage;
    d.size.width = desc->size.width;
    d.size.height = desc->size.height;
    d.size.depthOrArrayLayers = desc->size.depthOrArrayLayers;
    d.format = static_cast<WGPUTextureFormat>(desc->format);
    d.mipLevelCount = desc->mipLevelCount;
    d.sampleCount = desc->sampleCount;
    d.dimension = static_cast<WGPUTextureDimension>(desc->dimension);
    return Texture(wgpuDeviceCreateTexture(handle_, &d));
}

inline Sampler Device::CreateSampler(const SamplerDescriptor* desc) const {
    WGPUSamplerDescriptor d{};
    d.label = desc->label;
    d.addressModeU = static_cast<WGPUAddressMode>(desc->addressModeU);
    d.addressModeV = static_cast<WGPUAddressMode>(desc->addressModeV);
    d.addressModeW = static_cast<WGPUAddressMode>(desc->addressModeW);
    d.magFilter = static_cast<WGPUFilterMode>(desc->magFilter);
    d.minFilter = static_cast<WGPUFilterMode>(desc->minFilter);
    d.mipmapFilter = static_cast<WGPUMipmapFilterMode>(desc->mipmapFilter);
    d.maxAnisotropy = desc->maxAnisotropy;
    return Sampler(wgpuDeviceCreateSampler(handle_, &d));
}

inline BindGroupLayout Device::CreateBindGroupLayout(const BindGroupLayoutDescriptor* desc) const {
  WGPUBindGroupLayoutEntry* entries = new WGPUBindGroupLayoutEntry[desc->entryCount];
  for (uint32_t i = 0; i < desc->entryCount; ++i) {
    entries[i] = {};
    entries[i].binding = desc->entries[i].binding;
    entries[i].visibility = desc->entries[i].visibility;
    entries[i].buffer.type = static_cast<WGPUBufferBindingType>(desc->entries[i].buffer.type);
    entries[i].texture.sampleType = static_cast<WGPUTextureSampleType>(desc->entries[i].texture.sampleType);
    entries[i].texture.viewDimension = static_cast<WGPUTextureViewDimension>(desc->entries[i].texture.viewDimension);
    entries[i].sampler.type = static_cast<WGPUSamplerBindingType>(desc->entries[i].sampler.type);
  }
  
  WGPUBindGroupLayoutDescriptor wgpu_desc = {};
  wgpu_desc.entryCount = desc->entryCount;
  wgpu_desc.entries = entries;
  
  auto result = BindGroupLayout(wgpuDeviceCreateBindGroupLayout(handle_, &wgpu_desc));
  delete[] entries;
  return result;
}

inline BindGroup Device::CreateBindGroup(const BindGroupDescriptor* desc) const {
  WGPUBindGroupEntry* entries = new WGPUBindGroupEntry[desc->entryCount];
  for (uint32_t i = 0; i < desc->entryCount; ++i) {
    entries[i] = {};
    entries[i].binding = desc->entries[i].binding;
    entries[i].buffer = desc->entries[i].buffer;
    entries[i].size = desc->entries[i].size;
    entries[i].textureView = desc->entries[i].textureView;
    entries[i].sampler = desc->entries[i].sampler;
  }
  
  WGPUBindGroupDescriptor wgpu_desc = {};
  wgpu_desc.layout = desc->layout;
  wgpu_desc.entryCount = desc->entryCount;
  wgpu_desc.entries = entries;
  
  auto result = BindGroup(wgpuDeviceCreateBindGroup(handle_, &wgpu_desc));
  delete[] entries;
  return result;
}

inline PipelineLayout Device::CreatePipelineLayout(const PipelineLayoutDescriptor* desc) const {
  WGPUPipelineLayoutDescriptor wgpu_desc = {};
  wgpu_desc.bindGroupLayoutCount = desc->bindGroupLayoutCount;
  wgpu_desc.bindGroupLayouts = desc->bindGroupLayouts;
  return PipelineLayout(wgpuDeviceCreatePipelineLayout(handle_, &wgpu_desc));
}

inline RenderPipeline Device::CreateRenderPipeline(const RenderPipelineDescriptor* desc) const {
  WGPURenderPipelineDescriptor wgpu_desc = {};
  wgpu_desc.layout = desc->layout;
  
  WGPUVertexState vertex = {};
  vertex.module = desc->vertex.module;
  vertex.entryPoint = desc->vertex.entryPoint;
  vertex.bufferCount = desc->vertex.bufferCount;
  vertex.buffers = (const WGPUVertexBufferLayout*)desc->vertex.buffers;
  wgpu_desc.vertex = vertex;
  
  WGPUFragmentState fragment = {};
  fragment.module = desc->fragment->module;
  fragment.entryPoint = desc->fragment->entryPoint;
  fragment.targetCount = desc->fragment->targetCount;
  
  // NOTE: This simplified binding supports only 1 target for now properly in this minimal wrapper
  // unless we copy arrays. For MPW-WGPU we use 1 target.
  std::vector<WGPUColorTargetState> targets(desc->fragment->targetCount);
  std::vector<WGPUBlendState> blends(desc->fragment->targetCount);

  for(uint32_t i=0; i<desc->fragment->targetCount; ++i) {
      targets[i] = {};
      targets[i].format = static_cast<WGPUTextureFormat>(desc->fragment->targets[i].format);
      targets[i].writeMask = static_cast<WGPUColorWriteMask>(desc->fragment->targets[i].writeMask);
      
      if (desc->fragment->targets[i].blend) {
          blends[i].color.operation = static_cast<WGPUBlendOperation>(
              desc->fragment->targets[i].blend->color.operation);
          blends[i].color.srcFactor = static_cast<WGPUBlendFactor>(
              desc->fragment->targets[i].blend->color.srcFactor);
          blends[i].color.dstFactor = static_cast<WGPUBlendFactor>(
              desc->fragment->targets[i].blend->color.dstFactor);
              
          blends[i].alpha.operation = static_cast<WGPUBlendOperation>(
              desc->fragment->targets[i].blend->alpha.operation);
          blends[i].alpha.srcFactor = static_cast<WGPUBlendFactor>(
              desc->fragment->targets[i].blend->alpha.srcFactor);
          blends[i].alpha.dstFactor = static_cast<WGPUBlendFactor>(
              desc->fragment->targets[i].blend->alpha.dstFactor);
              
          targets[i].blend = &blends[i];
      }
  }
  
  fragment.targets = targets.data();
  wgpu_desc.fragment = &fragment;
  
  wgpu_desc.primitive.topology = static_cast<WGPUPrimitiveTopology>(desc->primitive.topology);
  wgpu_desc.primitive.cullMode = static_cast<WGPUCullMode>(desc->primitive.cullMode);
  wgpu_desc.multisample.count = desc->multisample.count;
  wgpu_desc.multisample.mask = ~0u; // Enable all samples!
  wgpu_desc.multisample.alphaToCoverageEnabled = false;
  
  return RenderPipeline(wgpuDeviceCreateRenderPipeline(handle_, &wgpu_desc));
}

inline CommandEncoder Device::CreateCommandEncoder(const WGPUCommandEncoderDescriptor* desc) const {
    return CommandEncoder(wgpuDeviceCreateCommandEncoder(handle_, desc));
}

inline Queue Device::GetQueue() const {
  return Queue(wgpuDeviceGetQueue(handle_));
}

// Adapter
inline Device Adapter::RequestDevice(const WGPUDeviceDescriptor* desc) const {
    struct UserData {
        WGPUDevice device = nullptr;
        bool done = false; 
    };
    UserData data;
    
    auto callback = [](WGPURequestDeviceStatus status, WGPUDevice device, const char* message, void* userdata) {
        UserData* d = static_cast<UserData*>(userdata);
        if (status == WGPURequestDeviceStatus_Success) {
            d->device = device;
        } else if (message) {
            printf("Device Request Failed: %s\n", message);
        }
    };
    
    wgpuAdapterRequestDevice(handle_, desc, callback, &data);
    return Device(data.device);
}

// Instance
inline Surface Instance::CreateSurface(const WGPUSurfaceDescriptor* desc) const {
    return Surface(wgpuInstanceCreateSurface(handle_, desc));
}

inline Adapter Instance::RequestAdapter(const WGPURequestAdapterOptions* options) const {
   struct UserData {
        WGPUAdapter adapter = nullptr;
    };
    UserData data;
    
    auto callback = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, const char* message, void* userdata) {
        UserData* d = static_cast<UserData*>(userdata);
        if (status == WGPURequestAdapterStatus_Success) {
            d->adapter = adapter;
        } else if (message) {
            printf("Adapter Request Failed: %s\n", message);
        }
    };
    
    wgpuInstanceRequestAdapter(handle_, options, callback, &data);
    return Adapter(data.adapter);
}

}  // namespace wgpu

#endif  // MPL_WGPU_WEBGPU_HPP_
