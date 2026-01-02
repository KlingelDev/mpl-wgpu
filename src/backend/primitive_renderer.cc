// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

#include "primitive_renderer.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>

#define STB_TRUETYPE_IMPLEMENTATION
#include "vendor/stb_truetype.h"

namespace mpl_wgpu {

namespace {
// ... LoadShader ...
std::string LoadShader() {
  // Try multiple paths (from project root, from build dir, etc.)
  std::vector<std::string> paths = {
    "src/backend/primitives.wgsl",
    "../src/backend/primitives.wgsl",
    "../../src/backend/primitives.wgsl",
    "../../../src/backend/primitives.wgsl",
     // Absolute fallback for typical project structure
  };
  
  for (const auto& path : paths) {
    std::ifstream file(path);
    if (file.is_open()) {
      std::stringstream buffer;
      buffer << file.rdbuf();
      return buffer.str();
    }
  }
  
  throw std::runtime_error(
      "Failed to load primitives.wgsl shader. "
      "Searched: src/backend/, ../src/backend/, ../../src/backend/, "
      "../../../src/backend/");
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
  // IMPORTANT: Store shader code in a local variable to prevent dangling pointer
  std::string shader_code = LoadShader();
  
  wgpu::ShaderModuleWGSLDescriptor wgsl_desc{};
  wgsl_desc.code = shader_code.c_str();
  
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

  // Create bind group layout (Uniform + Texture + Sampler)
  std::vector<wgpu::BindGroupLayoutEntry> bg_entries_layout(3);
  
  // 0: Uniforms
  bg_entries_layout[0].binding = 0;
  bg_entries_layout[0].visibility = static_cast<uint32_t>(wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment);
  bg_entries_layout[0].buffer.type = wgpu::BufferBindingType::Uniform;

  // 1: Texture (Font)
  bg_entries_layout[1].binding = 1;
  bg_entries_layout[1].visibility = static_cast<uint32_t>(wgpu::ShaderStage::Fragment);
  bg_entries_layout[1].texture.sampleType = wgpu::TextureSampleType::Float;
  bg_entries_layout[1].texture.viewDimension = wgpu::TextureViewDimension::e2D;

  // 2: Sampler
  bg_entries_layout[2].binding = 2;
  bg_entries_layout[2].visibility = static_cast<uint32_t>(wgpu::ShaderStage::Fragment);
  bg_entries_layout[2].sampler.type = wgpu::SamplerBindingType::Filtering;

  wgpu::BindGroupLayoutDescriptor bg_layout_desc{};
  bg_layout_desc.entryCount = 3;
  bg_layout_desc.entries = bg_entries_layout.data();
  bind_group_layout_ = device.CreateBindGroupLayout(&bg_layout_desc);

  // Create Dummy Texture (1x1 White)
  wgpu::TextureDescriptor tex_desc{};
  tex_desc.size = {1, 1, 1};
  tex_desc.format = wgpu::TextureFormat::R8Unorm;
  tex_desc.usage = static_cast<uint32_t>(wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst);
  tex_desc.sampleCount = 1;
  tex_desc.mipLevelCount = 1;
  tex_desc.dimension = static_cast<uint32_t>(wgpu::TextureDimension::e2D);
  tex_desc.label = "DummyFontAtlas";
  font_texture_ = device.CreateTexture(&tex_desc);
  
  uint8_t white = 255;
  wgpu::ImageCopyTextureWgpu dest{}; dest.texture = font_texture_;
  wgpu::TextureDataLayoutWgpu layout{}; layout.bytesPerRow = 1; layout.rowsPerImage = 1;
  device.GetQueue().WriteTexture(&dest, &white, 1, &layout, &tex_desc.size);

  // Create Sampler
  wgpu::SamplerDescriptor sampler_desc{};
  sampler_desc.magFilter = wgpu::FilterMode::Linear;
  sampler_desc.minFilter = wgpu::FilterMode::Linear;
  sampler_desc.maxAnisotropy = 1;
  font_sampler_ = device.CreateSampler(&sampler_desc);

  // Recreate BindGroup with new texture (Font Atlas)
  // Create BindGroup
  std::vector<wgpu::BindGroupEntry> bg_entries(3);
  
  bg_entries[0].binding = 0;
  bg_entries[0].buffer = uniform_buffer_.Get();
  bg_entries[0].size = 96; 

  bg_entries[1].binding = 1;
  bg_entries[1].textureView = font_texture_.CreateView();

  bg_entries[2].binding = 2;
  bg_entries[2].sampler = font_sampler_.Get();

  wgpu::BindGroupDescriptor bg_desc{};
  bg_desc.layout = bind_group_layout_.Get();
  bg_desc.entryCount = 3;
  bg_desc.entries = bg_entries.data();
  bind_group_ = device.CreateBindGroup(&bg_desc);

  // Create pipeline layout
  WGPUBindGroupLayout raw_bg_layout = bind_group_layout_.Get();
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

void PrimitiveRenderer::DrawRects(const std::vector<Rect>& rects,
                                  float screen_width, float screen_height) {
  // TODO: Handle screen size scaling if needed, for now assume 1:1 or handled by Viewport/Matrix
  // Matplot++ sends pixel coordinates usually
  
  for (const auto& r : rects) {
    DrawRect(r.x, r.y, r.width, r.height, {r.r, r.g, r.b, r.a}, r.corner_radius, r.stroke_width);
  }
}

void PrimitiveRenderer::DrawLines(const std::vector<Line>& lines,
                                  float screen_width, float screen_height) {
  for (const auto& l : lines) {
    DrawLine(l.x1, l.y1, l.z1, l.x2, l.y2, l.z2, l.width, {l.r, l.g, l.b, l.a}, l.dash_len, l.gap_len, l.dash_offset);
  }
}

void PrimitiveRenderer::DrawCircles(const std::vector<Circle>& circles,
                                    float screen_width, float screen_height) {
  for (const auto& c : circles) {
     // TODO: Handle marker types properly. For now assuming circles.
     float stroke = 0.0f; // Filled by default? Or line?
     // Check primitives.wgsl logic for circle
     DrawCircle(c.cx, c.cy, c.cz, c.radius, {c.r, c.g, c.b, c.a}, stroke);
  }
}

void PrimitiveRenderer::DrawTriangles(const std::vector<Triangle>& triangles,
                                      float screen_width, float screen_height) {
  for (const auto& t : triangles) {
      DrawTriangle(t.x1, t.y1, t.z1, t.x2, t.y2, t.z2, t.x3, t.y3, t.z3, 
                   {t.r, t.g, t.b, t.a}, true);
  }
}


void PrimitiveRenderer::Clear(const std::array<float, 4>& color) {
    // This Clear is called by Backend::new_frame usually.
    // The actual framebuffer clear happens in RenderPass.
    // We just clear our batch.
    Clear();
}

void PrimitiveRenderer::DrawImage(const std::vector<float>& data,
                                  size_t img_width, size_t img_height,
                                  float x, float y, float width, float height) {
    if (data.empty()) return;

    // 1. Create Texture for this image (R8Unorm)
    wgpu::TextureDescriptor tex_desc{};
    tex_desc.size = {static_cast<uint32_t>(img_width), static_cast<uint32_t>(img_height), 1};
    tex_desc.format = wgpu::TextureFormat::R8Unorm;
    tex_desc.usage = static_cast<uint32_t>(wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst);
    tex_desc.sampleCount = 1;
    tex_desc.mipLevelCount = 1;
    tex_desc.dimension = static_cast<uint32_t>(wgpu::TextureDimension::e2D);
    
    wgpu::Texture texture = device_.CreateTexture(&tex_desc);

    // 2. Upload Data (Convert Float 0..1 to Byte 0..255)
    std::vector<uint8_t> bytes(data.size());
    for(size_t i = 0; i < data.size(); ++i) {
        bytes[i] = static_cast<uint8_t>(std::clamp(data[i] * 255.0f, 0.0f, 255.0f));
    }
    
    wgpu::ImageCopyTextureWgpu dest{}; 
    dest.texture = texture;
    wgpu::TextureDataLayoutWgpu layout{}; 
    layout.bytesPerRow = static_cast<uint32_t>(img_width); 
    layout.rowsPerImage = static_cast<uint32_t>(img_height);
    
    device_.GetQueue().WriteTexture(&dest, bytes.data(), bytes.size(), &layout, &tex_desc.size);

    // 3. Create BindGroup
    std::vector<wgpu::BindGroupEntry> bg_entries(3);
    bg_entries[0].binding = 0;
    bg_entries[0].buffer = uniform_buffer_.Get();
    bg_entries[0].size = 96;

    bg_entries[1].binding = 1;
    bg_entries[1].textureView = texture.CreateView();

    bg_entries[2].binding = 2;
    bg_entries[2].sampler = font_sampler_.Get(); // Reuse sampler

    wgpu::BindGroupDescriptor bg_desc{};
    bg_desc.layout = bind_group_layout_.Get();
    bg_desc.entryCount = 3;
    bg_desc.entries = bg_entries.data();
    wgpu::BindGroup bind_group = device_.CreateBindGroup(&bg_desc);

    // 4. Create Instance Data (Textured Rect)
    float center_x = x + width * 0.5f;
    float center_y = y + height * 0.5f;

    Instance inst{};
    inst.pos_a_radius[0] = center_x;
    inst.pos_a_radius[1] = center_y;
    inst.pos_a_radius[2] = 0.5f; // Z?
    inst.pos_a_radius[3] = 0.0f;
    
    inst.pos_b_width[0] = width;
    inst.pos_b_width[1] = height;
    inst.pos_b_width[2] = 0.0f;
    inst.pos_b_width[3] = 0.0f;
    
    // UVs are full 0..1 by default in shader if not text?
    // Use params to indicate "It's a Text/Image"
    // kText = 100.
    inst.params[0] = 100.0f; // Treat as Text (Textured Rect)
    
    // Pass UVs (Full Texture)
    inst.pos_c_pad[0] = 0.0f;
    inst.pos_c_pad[1] = 0.0f;
    inst.pos_c_pad[2] = 1.0f;
    inst.pos_c_pad[3] = 1.0f;
    
    // Color tint white
    inst.color[0] = 1.0f; inst.color[1] = 1.0f; inst.color[2] = 1.0f; inst.color[3] = 1.0f;

    PendingImage pi;
    pi.texture = texture;
    pi.bind_group = bind_group;
    pi.instance_data = inst;
    
    pending_images_.push_back(pi);
}

void PrimitiveRenderer::DrawRect(float x, float y, float width,
                                 float height,
                                 const std::array<float, 4>& color,
                                 float radius, float stroke_width, float z) {
  // Low Level Debug
  // if (width > 100) {
  //    std::cout << "PR::DrawRect: " << x << "," << y << " " << width << "x" << height << std::endl;
  // }

  // Convert Top-Left Corner to Center for Shader (which draws centered)
  float center_x = x + width * 0.5f;
  float center_y = y + height * 0.5f;

  Instance inst{};
  inst.pos_a_radius[0] = center_x;
  inst.pos_a_radius[1] = center_y;
  inst.pos_a_radius[2] = z; // Use Z
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

void PrimitiveRenderer::InitText() {
  if (font_initialized_) return;

  const char* font_path = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"; // Fallback to common linux font
  std::ifstream f(font_path, std::ios::binary);
  if (!f.is_open()) {
      std::cerr << "Failed to load font: " << font_path << std::endl;
      // Try local fallback?
      return;
  }
  
  std::vector<unsigned char> ttf_buffer((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  f.close();

  // Bake Font
  int width = 512;
  int height = 512;
  std::vector<unsigned char> temp_bitmap(width * height);
  cdata_.resize(96); // ASCII 32..126

  stbtt_BakeFontBitmap(ttf_buffer.data(), 0, 32.0f, temp_bitmap.data(), width, height, 32, 96, reinterpret_cast<stbtt_bakedchar*>(cdata_.data()));

  // Create Texture
  wgpu::TextureDescriptor tex_desc{};
  tex_desc.size = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
  tex_desc.format = wgpu::TextureFormat::R8Unorm;
  tex_desc.usage = static_cast<uint32_t>(wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst);
  tex_desc.sampleCount = 1;
  tex_desc.mipLevelCount = 1;
  tex_desc.dimension = static_cast<uint32_t>(wgpu::TextureDimension::e2D);
  tex_desc.label = "FontAtlas";
  font_texture_ = device_.CreateTexture(&tex_desc);

  // Upload
  wgpu::ImageCopyTextureWgpu destination{};
  destination.texture = font_texture_;
  wgpu::TextureDataLayoutWgpu layout{};
  layout.bytesPerRow = width;
  layout.rowsPerImage = height;
  device_.GetQueue().WriteTexture(&destination, temp_bitmap.data(), temp_bitmap.size(), &layout, &tex_desc.size);

  // Create Sampler
  wgpu::SamplerDescriptor sampler_desc{};
  sampler_desc.magFilter = wgpu::FilterMode::Linear;
  sampler_desc.minFilter = wgpu::FilterMode::Linear;
  sampler_desc.maxAnisotropy = 1;
  font_sampler_ = device_.CreateSampler(&sampler_desc);

  font_initialized_ = true;
  font_initialized_ = true;
  std::cout << "Font Atlas Initialized!" << std::endl;

  // Recreate BindGroup
  std::vector<wgpu::BindGroupEntry> bg_entries(3);
  bg_entries[0].binding = 0;
  bg_entries[0].buffer = uniform_buffer_.Get();
  bg_entries[0].size = 96;
  
  bg_entries[1].binding = 1;
  bg_entries[1].textureView = font_texture_.CreateView();

  bg_entries[2].binding = 2;
  bg_entries[2].sampler = font_sampler_.Get();

  wgpu::BindGroupDescriptor bg_desc{};
  bg_desc.layout = bind_group_layout_.Get();
  bg_desc.entryCount = 3;
  bg_desc.entries = bg_entries.data();
  bind_group_ = device_.CreateBindGroup(&bg_desc);
}

void PrimitiveRenderer::DrawText(const std::string& text, float x, float y,
                                 float font_size, const std::array<float, 4>& color,
                                 float rotation) {
  if (!font_initialized_) InitText();
  if (!font_initialized_) {
    return;
  }

  float cur_x = x;
  float cur_y = y;

  // Assume baked at 32px. Scale accordingly.
  float scale = font_size / 32.0f;
  
  for (char c : text) {
      if (c < 32 || c > 126) continue;
      
      StbBakedChar* b = &cdata_[c - 32];
      
      float xpos = cur_x + b->xoff * scale;
      float ypos = cur_y + b->yoff * scale;
      float w = (b->x1 - b->x0) * scale;
      float h = (b->y1 - b->y0) * scale;
      
      // UVs
      float u0 = b->x0 / 512.0f;
      float v0 = b->y0 / 512.0f;
      float u1 = b->x1 / 512.0f;
      float v1 = b->y1 / 512.0f;

      // Draw Textured Rect
      // We start at Top-Left. 
      // PrimitiveRenderer::DrawRect takes center.
      // But we can re-use the generic Instance struct if we add UV support.
      // Instance has pos_c_pad (location 4).
      
      float center_x = xpos + w * 0.5f;
      float center_y = ypos + h * 0.5f;

      Instance inst{};
      inst.pos_a_radius[0] = center_x;
      inst.pos_a_radius[1] = center_y;
      inst.pos_a_radius[2] = 0.95f; // Text Front?
      inst.pos_a_radius[3] = 0.0f; // Radius 0
      
      inst.pos_b_width[0] = w;
      inst.pos_b_width[1] = h;
      inst.pos_b_width[2] = 0.0f;
      inst.pos_b_width[3] = 0.0f; // Stroke 0
      
      std::memcpy(inst.color, color.data(), sizeof(inst.color));
      
      inst.params[0] = static_cast<float>(PrimitiveType::kText); // Type 100
      inst.params[1] = 0;
      inst.params[2] = 0;
      inst.params[3] = 0;

      // Pass UVs in pos_c_pad (Location 4)
      inst.pos_c_pad[0] = u0;
      inst.pos_c_pad[1] = v0;
      inst.pos_c_pad[2] = u1;
      inst.pos_c_pad[3] = v1;

      instances_.push_back(inst);

      cur_x += b->xadvance * scale;
  }
}

float PrimitiveRenderer::MeasureText(const std::string& text, float font_size) {
  if (!font_initialized_) InitText();
  if (!font_initialized_) return 0.0f;

  float width = 0.0f;
  float scale = font_size / 32.0f;
  
  for (char c : text) {
      if (c < 32 || c > 126) continue;
      StbBakedChar* b = &cdata_[c - 32];
      width += b->xadvance * scale;
  }
  return width;
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
  // Stable sort to preserve Painter's Algorithm for non-faces
  std::stable_sort(instances_.begin(), instances_.end(),
            [](const Instance& a, const Instance& b) {
              uint32_t type_a = static_cast<uint32_t>(a.params[0]);
              uint32_t type_b = static_cast<uint32_t>(b.params[0]);
              // Faces: Rect(0), Circle(1), Triangle(30), TriangleUnlit(31)
              // Lines(2), Text(100) are drawn after
              auto is_face = [](uint32_t t) { 
                  return t == 0 || t == 1 || t == 30 || t == 31; 
              };
              bool is_face_a = is_face(type_a);
              bool is_face_b = is_face(type_b);
              // Faces come first
              return is_face_a && !is_face_b;
            });

  // Resize buffer if needed (use tracked capacity_, not GetSize which is stub)
  size_t total_instances = instances_.size() + pending_images_.size();
  if (total_instances > capacity_) {
    ResizeInstanceBuffer(total_instances + 128);
  }

  // Upload Normal Instances
  if (!instances_.empty()) {
      queue.WriteBuffer(instance_buffer_, 0, instances_.data(),
                        instances_.size() * sizeof(Instance));
  }

  // Upload Image Instances (Append)
  size_t offset = instances_.size();
  for (size_t i = 0; i < pending_images_.size(); ++i) {
      queue.WriteBuffer(instance_buffer_, (offset + i) * sizeof(Instance), 
                        &pending_images_[i].instance_data, sizeof(Instance));
  }
}

void PrimitiveRenderer::Render(wgpu::RenderPassEncoder& pass) {
  std::cerr << "PrimitiveRenderer::Render: " << instances_.size() << " instances\n";
  if (instances_.empty()) {
    return;
  }
  
  // DEBUG: Print instance count once per few frames
  // static int frame_count = 0;
  // if (frame_count++ % 60 == 0) {
  //     std::cout << "Render: " << instances_.size() << " instances" << std::endl;
  // }

  pass.SetBindGroup(0, bind_group_);
  pass.SetVertexBuffer(0, instance_buffer_);

  // Find split point between faces and other primitives
  auto split_it = std::find_if(
      instances_.begin(), instances_.end(), [](const Instance& inst) {
        uint32_t type = static_cast<uint32_t>(inst.params[0]);
        // Return true if NOT a face (i.e. first Line/Text)
        return type != 0 && type != 1 && type != 30 && type != 31;
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
  
  // Draw Images
  if (!pending_images_.empty()) {
      pass.SetPipeline(pipeline_lines_); // Use generic pipeline
      size_t base_instance = instances_.size();
      for (size_t i = 0; i < pending_images_.size(); ++i) {
          pass.SetBindGroup(0, pending_images_[i].bind_group);
          pass.Draw(6, 1, 0, base_instance + i);
      }
      
      // Restore Default BindGroup for safety
      pass.SetBindGroup(0, bind_group_);
  }
} // End Render


void PrimitiveRenderer::Clear() {
  instances_.clear();
  pending_images_.clear();
}

void PrimitiveRenderer::UpdateUniforms(wgpu::Queue queue) {
  // Create identity matrix + screen size
  // Shader GlobalUniforms layout:
  //   mat4x4<f32> view_proj  (64 bytes, 16 floats @ offset 0)
  //   vec2<f32> screen_size  (8 bytes, 2 floats @ offset 64/4=16)
  //   vec2<f32> _padding     (8 bytes @ offset 18)
  //   vec3<f32> camera_pos   (12 bytes, padded to 16 @ offset 20)
  // Total: 96 bytes = 24 floats
  float data[24] = {0};
  
  // Orthographic Projection Matrix (Column-Major)
  // Maps 0..width, 0..height to -1..-1, 1..-1 (flip Y)
  // Left=0, Right=width, Bottom=height, Top=0, Near=0, Far=1000
  
  float L = 0.0f;
  float R = static_cast<float>(width_);
  float B = static_cast<float>(height_);
  float T = 0.0f;
  float near = 0.0f;
  float far = 1000.0f;
  
  // m00 = 2 / (R - L)
  data[0] = 2.0f / (R - L);
  // m11 = 2 / (T - B)
  data[5] = 2.0f / (T - B);
  // m22 = 1 / (far - near) (Standard WebGPU 0..1)
  data[10] = 1.0f / (far - near); 
  // m33
  data[15] = 1.0f;
  
  // m30 = -(R + L) / (R - L)
  data[12] = -(R + L) / (R - L);
  // m31 = -(T + B) / (T - B)
  data[13] = -(T + B) / (T - B);
  // m32 = -near / (far - near)
  data[14] = -near / (far - near);
  
  // Screen size at offset 16
  data[16] = static_cast<float>(width_);
  data[17] = static_cast<float>(height_);
  
  // Padding at 18, 19
  // Camera pos at 20, 21, 22
  
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
