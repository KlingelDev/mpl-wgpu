// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

/// Minimal C++ wrapper for WebGPU C API
/// Provides wgpu:: namespace for ergonomic usage

#ifndef MPL_WGPU_WEBGPU_HPP_
#define MPL_WGPU_WEBGPU_HPP_

#include <webgpu.h>
#include <cstddef>
#include <cstdint>

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

// Enums
enum class TextureFormat : uint32_t {
  BGRA8Unorm = WGPUTextureFormat_BGRA8Unorm,
  RGBA8Unorm = WGPUTextureFormat_RGBA8Unorm,
};

enum class BufferUsage : uint32_t {
  Uniform = WGPUBufferUsage_Uniform,
  Vertex = WGPUBufferUsage_Vertex,
  CopyDst = WGPUBufferUsage_CopyDst,
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

// Descriptors (simplified)
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

struct BindGroupLayoutEntry {
  uint32_t binding;
  uint32_t visibility;
  struct {
    BufferBindingType type;
  } buffer;
};

struct BindGroupLayoutDescriptor {
  uint32_t entryCount;
  const BindGroupLayoutEntry* entries;
};

struct BindGroupEntry {
  uint32_t binding;
  WGPUBuffer buffer;
  uint64_t size;
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
  uint32_t attributeCount;
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

// Buffer class
class Buffer {
 public:
  Buffer() : handle_(nullptr) {}
  explicit Buffer(WGPUBuffer h) : handle_(h) {}
  
  WGPUBuffer Get() const { return handle_; }
  uint64_t GetSize() const { 
    return 0; // TODO: track size
  }
  
 private:
  WGPUBuffer handle_;
};

// Device class  
class Device {
 public:
  Device() : handle_(nullptr) {}
  explicit Device(WGPUDevice h) : handle_(h) {}
  
  WGPUDevice Get() const { return handle_; }
  
  ShaderModule CreateShaderModule(
      const ShaderModuleDescriptor* desc) const;
  Buffer CreateBuffer(const BufferDescriptor* desc) const;
  BindGroupLayout CreateBindGroupLayout(
      const BindGroupLayoutDescriptor* desc) const;
  BindGroup CreateBindGroup(
      const BindGroupDescriptor* desc) const;
  PipelineLayout CreatePipelineLayout(
      const void* desc) const;
  RenderPipeline CreateRenderPipeline(
      const void* desc) const;
      
  Queue GetQueue() const;
  
 private:
  WGPUDevice handle_;
};

// Queue class
class Queue {
 public:
  Queue() : handle_(nullptr) {}
  explicit Queue(WGPUQueue h) : handle_(h) {}
  
  void WriteBuffer(Buffer buffer, uint64_t offset,
                   const void* data, size_t size) const {
    wgpuQueueWriteBuffer(handle_, buffer.Get(), offset, data, size);
  }
  
 private:
  WGPUQueue handle_;
};

// Inline implementations
inline ShaderModule Device::CreateShaderModule(
    const ShaderModuleDescriptor* desc) const {
  WGPUShaderModuleWGSLDescriptor wgsl = {};
  wgsl.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
  wgsl.code = desc->nextInChain->code;
  
  WGPUShaderModuleDescriptor wgpu_desc = {};
  wgpu_desc.nextInChain = (const WGPUChainedStruct*)&wgsl;
  
  return ShaderModule(wgpuDeviceCreateShaderModule(handle_, &wgpu_desc));
}

inline Buffer Device::CreateBuffer(
    const BufferDescriptor* desc) const {
  WGPUBufferDescriptor wgpu_desc = {};
  wgpu_desc.label = desc->label;
  wgpu_desc.size = desc->size;
  wgpu_desc.usage = desc->usage;
  return Buffer(wgpuDeviceCreateBuffer(handle_, &wgpu_desc));
}

inline BindGroupLayout Device::CreateBindGroupLayout(
    const BindGroupLayoutDescriptor* desc) const {
  // Convert entries
  WGPUBindGroupLayoutEntry* entries = 
      new WGPUBindGroupLayoutEntry[desc->entryCount];
  for (uint32_t i = 0; i < desc->entryCount; ++i) {
    entries[i] = {};
    entries[i].binding = desc->entries[i].binding;
    entries[i].visibility = desc->entries[i].visibility;
    entries[i].buffer.type = static_cast<WGPUBufferBindingType>(
        desc->entries[i].buffer.type);
  }
  
  WGPUBindGroupLayoutDescriptor wgpu_desc = {};
  wgpu_desc.entryCount = desc->entryCount;
  wgpu_desc.entries = entries;
  
  auto result = BindGroupLayout(
      wgpuDeviceCreateBindGroupLayout(handle_, &wgpu_desc));
  delete[] entries;
  return result;
}

inline BindGroup Device::CreateBindGroup(
    const BindGroupDescriptor* desc) const {
  WGPUBindGroupEntry* entries = 
      new WGPUBindGroupEntry[desc->entryCount];
  for (uint32_t i = 0; i < desc->entryCount; ++i) {
    entries[i] = {};
    entries[i].binding = desc->entries[i].binding;
    entries[i].buffer = desc->entries[i].buffer;
    entries[i].size = desc->entries[i].size;
  }
  
  WGPUBindGroupDescriptor wgpu_desc = {};
  wgpu_desc.layout = desc->layout;
  wgpu_desc.entryCount = desc->entryCount;
  wgpu_desc.entries = entries;
  
  auto result = BindGroup(wgpuDeviceCreateBindGroup(handle_, &wgpu_desc));
  delete[] entries;
  return result;
}

inline PipelineLayout Device::CreatePipelineLayout(
    const PipelineLayoutDescriptor* desc) const {
  WGPUPipelineLayoutDescriptor wgpu_desc = {};
  wgpu_desc.bindGroupLayoutCount = desc->bindGroupLayoutCount;
  wgpu_desc.bindGroupLayouts = desc->bindGroupLayouts;
  
  return PipelineLayout(
      wgpuDeviceCreatePipelineLayout(handle_, &wgpu_desc));
}

inline RenderPipeline Device::CreateRenderPipeline(
    const RenderPipelineDescriptor* desc) const {
  // This is complex - just return null for now
  // TODO: Implement full conversion
  return RenderPipeline(nullptr);
}

inline Queue Device::GetQueue() const {
  return Queue(wgpuDeviceGetQueue(handle_));
}

// Placeholder classes
class ShaderModule {
 public:
  ShaderModule() : handle_(nullptr) {}
  explicit ShaderModule(WGPUShaderModule h) : handle_(h) {}
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

class RenderPassEncoder {
 public:
  RenderPassEncoder() : handle_(nullptr) {}
  explicit RenderPassEncoder(WGPURenderPassEncoder h) : handle_(h) {}
  
  void SetBindGroup(uint32_t index, BindGroup group, 
                    const uint32_t* offsets) const {
    wgpuRenderPassEncoderSetBindGroup(handle_, index, group.Get(), 
                                      0, offsets);
  }
  
  void SetVertexBuffer(uint32_t slot, Buffer buffer) const {
    wgpuRenderPassEncoderSetVertexBuffer(handle_, slot, buffer.Get(),
                                         0, buffer.GetSize());
  }
  
  void SetPipeline(RenderPipeline pipeline) const {
    wgpuRenderPassEncoderSetPipeline(handle_, pipeline.Get());
  }
  
  void Draw(uint32_t vertexCount, uint32_t instanceCount,
            uint32_t firstVertex, uint32_t firstInstance) const {
    wgpuRenderPassEncoderDraw(handle_, vertexCount, instanceCount,
                              firstVertex, firstInstance);
  }
  
 private:
  WGPURenderPassEncoder handle_;
};

}  // namespace wgpu

#endif  // MPL_WGPU_WEBGPU_HPP_
