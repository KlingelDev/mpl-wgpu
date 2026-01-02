// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

#include "primitive_renderer.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>

namespace mpl_wgpu {

namespace {

// Load WGSL shader from file
std::string LoadShader() {
  std::ifstream file("src/backend/primitives.wgsl");
  if (!file.is_open()) {
    // Try alternate path for build directory
    file.open("../src/backend/primitives.wgsl");
  }
  
  if (!file.is_open()) {
    throw std::runtime_error(
        "Failed to load primitives.wgsl shader");
  }
  
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

}  // namespace

PrimitiveRenderer::PrimitiveRenderer(wgpu::Device device,
                                     wgpu::TextureFormat format,
                                     uint32_t width,
                                     uint32_t height)
    : device_(device),
      width_(width),
      height_(height),
      capacity_(1024) {
  
  // Create shader module
  wgpu::ShaderModuleWGSLDescriptor wgsl_desc{};
  wgsl_desc.code = LoadShader().c_str();
  
  wgpu::ShaderModuleDescriptor shader_desc{};
  shader_desc.nextInChain = &wgsl_desc;
  wgpu::ShaderModule shader = device.CreateShaderModule(&shader_desc);

  // Create uniform buffer (Mat4 + Vec2 + padding)
  wgpu::BufferDescriptor uniform_buf_desc{};
  uniform_buf_desc.label = "PrimitiveUniforms";
  uniform_buf_desc.size = 64 + 16 + 16;  // Mat4 + Vec2 + Camera + pad
  uniform_buf_desc.usage = static_cast<uint32_t>(wgpu::BufferUsage::Uniform | 
                           wgpu::BufferUsage::CopyDst);
  uniform_buffer_ = device.CreateBuffer(&uniform_buf_desc);

  // Create bind group layout
  wgpu::BindGroupLayoutEntry bg_entry{};
  bg_entry.binding = 0;
  bg_entry.visibility = static_cast<uint32_t>(wgpu::ShaderStage::Vertex | 
                        wgpu::ShaderStage::Fragment);
  bg_entry.buffer.type = wgpu::BufferBindingType::Uniform;

  wgpu::BindGroupLayoutDescriptor bg_layout_desc{};
  bg_layout_desc.entryCount = 1;
  bg_layout_desc.entries = &bg_entry;
  wgpu::BindGroupLayout bg_layout = 
      device.CreateBindGroupLayout(&bg_layout_desc);

  // Create bind group
  wgpu::BindGroupEntry bg_bind_entry{};
  bg_bind_entry.binding = 0;
  bg_bind_entry.buffer = uniform_buffer_.Get();
  bg_bind_entry.size = uniform_buf_desc.size;

  wgpu::BindGroupDescriptor bg_desc{};
  bg_desc.layout = bg_layout.Get();
  bg_desc.entryCount = 1;
  bg_desc.entries = &bg_bind_entry;
  bind_group_ = device.CreateBindGroup(&bg_desc);

  // Create pipeline layout
  WGPUBindGroupLayout raw_bg_layout = bg_layout.Get();
  wgpu::PipelineLayoutDescriptor layout_desc{};
  layout_desc.bindGroupLayoutCount = 1;
  layout_desc.bindGroupLayouts = &raw_bg_layout;
  wgpu::PipelineLayout pipeline_layout = 
      device.CreatePipelineLayout(&layout_desc);

  // Vertex buffer layout (instance data)
  wgpu::VertexAttribute attributes[5];
  attributes[0].format = wgpu::VertexFormat::Float32x4;
  attributes[0].offset = 0;
  attributes[0].shaderLocation = 0;
  
  attributes[1].format = wgpu::VertexFormat::Float32x4;
  attributes[1].offset = 16;
  attributes[1].shaderLocation = 1;
  
  attributes[2].format = wgpu::VertexFormat::Float32x4;
  attributes[2].offset = 32;
  attributes[2].shaderLocation = 2;
  
  attributes[3].format = wgpu::VertexFormat::Float32x4;
  attributes[3].offset = 48;
  attributes[3].shaderLocation = 3;
  
  attributes[4].format = wgpu::VertexFormat::Float32x4;
  attributes[4].offset = 64;
  attributes[4].shaderLocation = 4;

  wgpu::VertexBufferLayout vertex_buffer{};
  vertex_buffer.arrayStride = sizeof(Instance);
  vertex_buffer.stepMode = wgpu::VertexStepMode::Instance;
  vertex_buffer.attributeCount = 5;
  vertex_buffer.attributes = attributes;

  // Create render pipelines (depth write + lines)
  wgpu::ColorTargetState color_target{};
  color_target.format = format;
  color_target.blend = new wgpu::BlendState{
      .color = {
          .operation = wgpu::BlendOperation::Add,
          .srcFactor = wgpu::BlendFactor::SrcAlpha,
          .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha,
      },
      .alpha = {
          .operation = wgpu::BlendOperation::Add,
          .srcFactor = wgpu::BlendFactor::One,
          .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha,
      },
  };
  color_target.writeMask = wgpu::ColorWriteMask::All;

  wgpu::FragmentState fragment{};
  fragment.module = shader.Get();
  fragment.entryPoint = "fs_main";
  fragment.targetCount = 1;
  fragment.targets = &color_target;

  wgpu::RenderPipelineDescriptor pipeline_desc{};
  pipeline_desc.layout = pipeline_layout.Get();
  pipeline_desc.vertex.module = shader.Get();
  pipeline_desc.vertex.entryPoint = "vs_main";
  pipeline_desc.vertex.bufferCount = 1;
  pipeline_desc.vertex.buffers = &vertex_buffer;
  pipeline_desc.primitive.topology = 
      wgpu::PrimitiveTopology::TriangleList;
  pipeline_desc.primitive.cullMode = wgpu::CullMode::None;
  pipeline_desc.fragment = &fragment;
  pipeline_desc.multisample.count = 1;

  pipeline_depth_write_ = device.CreateRenderPipeline(&pipeline_desc);
  pipeline_lines_ = device.CreateRenderPipeline(&pipeline_desc);

  // Create instance buffer
  wgpu::BufferDescriptor inst_buf_desc{};
  inst_buf_desc.label = "InstanceBuffer";
  inst_buf_desc.size = capacity_ * sizeof(Instance);
  inst_buf_desc.usage = static_cast<uint32_t>(wgpu::BufferUsage::Vertex | 
                        wgpu::BufferUsage::CopyDst);
  instance_buffer_ = device.CreateBuffer(&inst_buf_desc);

  UpdateUniforms(device.GetQueue());
}

void PrimitiveRenderer::Resize(uint32_t width, uint32_t height) {
  width_ = width;
  height_ = height;
  UpdateUniforms(device_.GetQueue());
}

void PrimitiveRenderer::DrawRect(float x, float y, float width,
                                 float height,
                                 const std::array<float, 4>& color,
                                 float radius, float stroke_width) {
  Instance inst{};
  inst.pos_a_radius[0] = x;
  inst.pos_a_radius[1] = y;
  inst.pos_a_radius[2] = 0.0f;
  inst.pos_a_radius[3] = radius;
  
  inst.pos_b_width[0] = width;
  inst.pos_b_width[1] = height;
  inst.pos_b_width[2] = 0.0f;
  inst.pos_b_width[3] = stroke_width;
  
  std::memcpy(inst.color, color.data(), sizeof(inst.color));
  
  inst.params[0] = 
      static_cast<float>(PrimitiveType::kRectangle);
  
  instances_.push_back(inst);
}

void PrimitiveRenderer::DrawLine(float x1, float y1, float z1,
                                 float x2, float y2, float z2,
                                 float thickness,
                                 const std::array<float, 4>& color,
                                 float dash_len, float gap_len,
                                 float dash_offset) {
  Instance inst{};
  inst.pos_a_radius[0] = x1;
  inst.pos_a_radius[1] = y1;
  inst.pos_a_radius[2] = z1;
  inst.pos_a_radius[3] = thickness * 0.5f;
  
  inst.pos_b_width[0] = x2;
  inst.pos_b_width[1] = y2;
  inst.pos_b_width[2] = z2;
  inst.pos_b_width[3] = 0.0f;
  
  std::memcpy(inst.color, color.data(), sizeof(inst.color));
  
  inst.params[0] = static_cast<float>(PrimitiveType::kLine);
  inst.params[1] = dash_len;
  inst.params[2] = gap_len;
  inst.params[3] = dash_offset;
  
  instances_.push_back(inst);
}

void PrimitiveRenderer::DrawCircle(float cx, float cy, float cz,
                                   float radius,
                                   const std::array<float, 4>& color,
                                   float stroke_width) {
  Instance inst{};
  inst.pos_a_radius[0] = cx;
  inst.pos_a_radius[1] = cy;
  inst.pos_a_radius[2] = cz;
  inst.pos_a_radius[3] = radius;
  
  inst.pos_b_width[3] = stroke_width;
  std::memcpy(inst.color, color.data(), sizeof(inst.color));
  
  inst.params[0] = static_cast<float>(PrimitiveType::kCircle);
  
  instances_.push_back(inst);
}

void PrimitiveRenderer::DrawTriangle(float x1, float y1, float z1,
                                     float x2, float y2, float z2,
                                     float x3, float y3, float z3,
                                     const std::array<float, 4>& color,
                                     bool lit) {
  Instance inst{};
  inst.pos_a_radius[0] = x1;
  inst.pos_a_radius[1] = y1;
  inst.pos_a_radius[2] = z1;
  
  inst.pos_b_width[0] = x2;
  inst.pos_b_width[1] = y2;
  inst.pos_b_width[2] = z2;
  
  std::memcpy(inst.color, color.data(), sizeof(inst.color));
  
  inst.params[0] = lit ? static_cast<float>(PrimitiveType::kTriangle)
                       : static_cast<float>(
                             PrimitiveType::kTriangleUnlit);
  
  inst.pos_c_pad[0] = x3;
  inst.pos_c_pad[1] = y3;
  inst.pos_c_pad[2] = z3;
  
  instances_.push_back(inst);
}

void PrimitiveRenderer::Prepare(wgpu::Queue queue) {
  if (instances_.empty()) {
    return;
  }

  // Sort: faces first, then others
  std::sort(instances_.begin(), instances_.end(),
            [](const Instance& a, const Instance& b) {
              uint32_t type_a = static_cast<uint32_t>(a.params[0]);
              uint32_t type_b = static_cast<uint32_t>(b.params[0]);
              bool is_face_a = (type_a == 30 || type_a == 31);
              bool is_face_b = (type_b == 30 || type_b == 31);
              return is_face_a && !is_face_b;
            });

  // Resize buffer if needed
  size_t required_size = instances_.size() * sizeof(Instance);
  if (required_size > instance_buffer_.GetSize()) {
    ResizeInstanceBuffer(instances_.size());
  }

  // Upload to GPU
  queue.WriteBuffer(instance_buffer_, 0, instances_.data(),
                    instances_.size() * sizeof(Instance));
}

void PrimitiveRenderer::Render(wgpu::RenderPassEncoder& pass) {
  if (instances_.empty()) {
    return;
  }

  pass.SetBindGroup(0, bind_group_);
  pass.SetVertexBuffer(0, instance_buffer_);

  // Find split point between faces and other primitives
  auto split_it = std::find_if(
      instances_.begin(), instances_.end(), [](const Instance& inst) {
        uint32_t type = static_cast<uint32_t>(inst.params[0]);
        return type != 30 && type != 31;
      });
  
  size_t split_idx = std::distance(instances_.begin(), split_it);

  // Draw faces (depth write)
  if (split_idx > 0) {
    pass.SetPipeline(pipeline_depth_write_);
    pass.Draw(6, split_idx, 0, 0);
  }

  // Draw lines/circles/etc
  if (split_idx < instances_.size()) {
    pass.SetPipeline(pipeline_lines_);
    pass.Draw(6, instances_.size() - split_idx, 0, split_idx);
  }
}

void PrimitiveRenderer::Clear() {
  instances_.clear();
}

void PrimitiveRenderer::UpdateUniforms(wgpu::Queue queue) {
  // Create identity matrix + screen size
  float data[24] = {0};
  
  // Identity matrix
  data[0] = 1.0f;
  data[5] = 1.0f;
  data[10] = 1.0f;
  data[15] = 1.0f;
  
  // Screen size
  data[16] = static_cast<float>(width_);
  data[17] = static_cast<float>(height_);
  
  queue.WriteBuffer(uniform_buffer_, 0, data, sizeof(data));
}

void PrimitiveRenderer::ResizeInstanceBuffer(size_t new_capacity) {
  capacity_ = new_capacity;
  
  wgpu::BufferDescriptor desc{};
  desc.label = "InstanceBuffer";
  desc.size = capacity_ * sizeof(Instance);
  desc.usage = static_cast<uint32_t>(wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst);
  
  instance_buffer_ = device_.CreateBuffer(&desc);
}

}  // namespace mpl_wgpu
