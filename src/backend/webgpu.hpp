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

enum class ShaderStage : uint32_t {
  Vertex = WGPUShaderStage_Vertex,
  Fragment = WGPUShaderStage_Fragment,
};

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

// Descriptors (simplified)
struct ShaderModuleWGSLDescriptor {
  const char* code;
};

struct ShaderModuleDescriptor {
  void* nextInChain;
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
    uint32_t type;
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
    const ShaderModuleDescriptor*) const {
  // TODO: Implement
  return ShaderModule();
}

inline Buffer Device::CreateBuffer(
    const BufferDescriptor* desc) const {
  WGPUBufferDescriptor wgpu_desc = {};
  wgpu_desc.label = desc->label;
  wgpu_desc.size = desc->size;
  wgpu_desc.usage = desc->usage;
  return Buffer(wgpuDeviceCreateBuffer(handle_, &wgpu_desc));
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
