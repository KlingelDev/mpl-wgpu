// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT
//
// WebGPU-based backend for Matplot++.

#include "matplot/backend/wgpu_backend.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>

namespace matplot::backend {

namespace {
// Matplot++ uses 1-based indexing for color_array {0, r, g, b}
// We need to map this to {r, g, b, 1.0}
inline std::array<float, 4> FixColor(const std::array<float, 4>& c) {
  if (std::isnan(c[1]) || std::isnan(c[2]) || std::isnan(c[3])) {
    return {0.0f, 0.0f, 0.0f, 0.0f};
  }
  return {c[1], c[2], c[3], 1.0f};
}

// Matplot++ passes colors as {Alpha, R, G, B} not {R, G, B, A}
inline std::array<float, 4> FixFillColor(const std::array<float, 4>& c) {
  if (std::isnan(c[1]) || std::isnan(c[2]) || std::isnan(c[3])) {
    return {0.0f, 0.0f, 0.0f, 0.0f};
  }
  // Matplot format is {Alpha/Unused, R, G, B}. 
  // We assume alpha is 1.0f unless implicit.
  // c[0] is often 0 or garbage for solid colors.
  return {c[1], c[2], c[3], 1.0f};
}
}

WgpuBackend::WgpuBackend(std::shared_ptr<WgpuRenderer> renderer)
    : renderer_(std::move(renderer)) {
  // Backend initialized
}

bool WgpuBackend::is_interactive() {
  return true;
}

bool WgpuBackend::new_frame() {
  rects_.clear();
  lines_.clear();
  circles_.clear();
  triangles_.clear();
  pending_segments_.clear();
  return true;
}

bool WgpuBackend::render_data() {
  if (!renderer_) return false;

  float w = static_cast<float>(render_width_);
  float h = static_cast<float>(render_height_);

  ReconstructRectangles();

  if (!rects_.empty()) renderer_->DrawRects(rects_, w, h);
  if (!lines_.empty()) renderer_->DrawLines(lines_, w, h);
  if (!circles_.empty()) renderer_->DrawCircles(circles_, w, h);
  if (!triangles_.empty()) renderer_->DrawTriangles(triangles_, w, h);
  
  if (!texts_.empty()) {
      for (const auto& t : texts_) {
          std::array<float, 4> c = {t.r, t.g, t.b, t.a};
          renderer_->DrawText(t.text, t.x, t.y, t.font_size, c, t.rotation);
      }
  }

  texts_.clear();

  rects_.clear();
  lines_.clear();
  circles_.clear();
  triangles_.clear();
  pending_segments_.clear();

  return true;
}

bool WgpuBackend::should_close() {
  return should_close_;
}

void WgpuBackend::show(matplot::figure_type* /*fig*/) {
  std::cout << "WgpuBackend::show() called!\n";
  render_data();
}

bool WgpuBackend::supports_fonts() {
  return true;
}

unsigned int WgpuBackend::width() { return width_; }
unsigned int WgpuBackend::height() { return height_; }
void WgpuBackend::width(unsigned int new_width) { 
    width_ = new_width; 
}
void WgpuBackend::height(unsigned int new_height) { 
    height_ = new_height; 
}
void WgpuBackend::set_render_size(unsigned int w, unsigned int h) {
  render_width_ = w;
  render_height_ = h;
}
unsigned int WgpuBackend::position_x() { return pos_x_; }
unsigned int WgpuBackend::position_y() { return pos_y_; }
void WgpuBackend::position_x(unsigned int x) { pos_x_ = x; }
void WgpuBackend::position_y(unsigned int y) { pos_y_ = y; }

void WgpuBackend::draw_background(const std::array<float, 4>& color) {
  std::array<float, 4> c = FixFillColor(color);

  WgpuRenderer::Rect r;
  r.x = 0.0f; r.y = 0.0f; 
  r.width = static_cast<float>(render_width_); 
  r.height = static_cast<float>(render_height_);
  r.r = c[0]; r.g = c[1]; r.b = c[2]; r.a = c[3];
  r.stroke_width = 0.0f;
  r.corner_radius = 0.0f;
  r.z = 0.9f; // Far (0..1 range)
  r.padding = 0.0f;
  rects_.push_back(r);
}

// Helper to center and scale content while preserving aspect ratio
void WgpuBackend::draw_rectangle(double x1, double x2, double y1, double y2, const std::array<float, 4>& color) {

  float rw = static_cast<float>(render_width_);
  float rh = static_cast<float>(render_height_);
  float lw = static_cast<float>(width_);
  float lh = static_cast<float>(height_);

  // Aspect Ratio Preserving Scale
  float scale = std::min(rw / lw, rh / lh);
  float offset_x = (rw - lw * scale) * 0.5f;
  float offset_y = (rh - lh * scale) * 0.5f;

  float px1 = static_cast<float>(x1) * scale + offset_x;
  float px2 = static_cast<float>(x2) * scale + offset_x;
  
  // Y-Flip with Centering:
  // Logical Y=0 -> Screen Bottom Valid Area (rh - offset_y)
  // Logical Y=lh -> Screen Top Valid Area (offset_y)
  // screen_y = (rh - offset_y) - (y * scale)
  float py1 = (rh - offset_y) - static_cast<float>(std::max(y1, y2)) * scale; // Top-most pixel (smallest Y)
  float py2 = (rh - offset_y) - static_cast<float>(std::min(y1, y2)) * scale; // Bottom-most pixel (largest Y)

  float left = std::min(px1, px2);
  float right = std::max(px1, px2);
  float top = py1;
  float bottom = py2;

  std::array<float, 4> c = FixFillColor(color);
  
  rects_.push_back({left, top, right - left, bottom - top, c[0], c[1], c[2], c[3], 0.0f, 0.0f, 0.0f, 0.0f});
}

void WgpuBackend::draw_triangle(const std::vector<double>& x,
                                const std::vector<double>& y,
                                const std::vector<double>& z) {
  if (x.size() < 3 || y.size() < 3) return;

  float rw = static_cast<float>(render_width_);
  float rh = static_cast<float>(render_height_);
  float lw = static_cast<float>(width_);
  float lh = static_cast<float>(height_);

  float scale = std::min(rw / lw, rh / lh);
  float offset_x = (rw - lw * scale) * 0.5f;
  float offset_y = (rh - lh * scale) * 0.5f;

  std::array<float, 4> c = marker_color_;

  // Transform vertices
  // Note: draw_triangle expects pre-transformed (logic->screen) coordinates usually if coming from axes_type?
  // But our axes_type logic (to be implemented) outputs SCALAR logic coordinates?
  // WgpuBackend applies the FINAL viewport transform (Scale + Offset).
  
  // So inputs x,y are 0..Width, 0..Height (Logical). Z is 0..1?
  // For 3D, Z needs to be handled.
  // We assume Z is already normalized by axes_type?
  // Or do we project?
  // PrimitiveRenderer uses Ortho projection.
  // We just pass it through.
  
  float tx1 = static_cast<float>(x[0]) * scale + offset_x;
  float ty1 = (rh - offset_y) - static_cast<float>(y[0]) * scale;
  float tz1 = z.size() > 0 ? static_cast<float>(z[0]) : 0.0f;

  float tx2 = static_cast<float>(x[1]) * scale + offset_x;
  float ty2 = (rh - offset_y) - static_cast<float>(y[1]) * scale;
  float tz2 = z.size() > 1 ? static_cast<float>(z[1]) : 0.0f;

  float tx3 = static_cast<float>(x[2]) * scale + offset_x;
  float ty3 = (rh - offset_y) - static_cast<float>(y[2]) * scale;
  float tz3 = z.size() > 2 ? static_cast<float>(z[2]) : 0.0f;

  triangles_.push_back({
      tx1, ty1, tz1, 0,
      tx2, ty2, tz2, 0,
      tx3, ty3, tz3, 0,
      0,0,1,0, 0,0,1,0, 0,0,1,0, // Normals placeholder
      c[0], c[1], c[2], c[3]
  });
}

// 3D Text Implementation (Matrix Projection)
// Logic: World Space Point -> Clip Space (via MVP) -> NDC (via /w) -> Screen Space
void WgpuBackend::draw_text_3d(const std::string& text, float x, float y, float z,
                           float font_size, const std::array<float, 4>& color) {
    if (!renderer_ || !has_view_proj_ || text.empty()) return;
    
    // Matrix transform (Column-Major)
    float cx = x * view_proj_[0] + y * view_proj_[4] + z * view_proj_[8] + view_proj_[12];
    float cy = x * view_proj_[1] + y * view_proj_[5] + z * view_proj_[9] + view_proj_[13];
    // float cz = ...; // Unused for now
    float cw = x * view_proj_[3] + y * view_proj_[7] + z * view_proj_[11] + view_proj_[15];

    if (cw <= 0.001f) return; // Behind camera

    float ndc_x = cx / cw;
    float ndc_y = cy / cw;

    // Viewport mapping
    float w = static_cast<float>(render_width_);
    float h = static_cast<float>(render_height_);
    
    float screen_x = (ndc_x + 1.0f) * 0.5f * w;
    float screen_y = (1.0f - ndc_y) * 0.5f * h;

    renderer_->DrawText(text, screen_x, screen_y, font_size, color, 0.0f);
}

void WgpuBackend::draw_path(const std::vector<double>& x, 
                             const std::vector<double>& y, 
                             const std::array<float, 4>& color) {
  size_t n = std::min(x.size(), y.size());
  if (n < 2) return;

  float rw = static_cast<float>(render_width_);
  float rh = static_cast<float>(render_height_);
  float lw = static_cast<float>(width_);
  float lh = static_cast<float>(height_);

  // Aspect Ratio Preserving Scale
  float scale = std::min(rw / lw, rh / lh);
  float offset_x = (rw - lw * scale) * 0.5f;
  float offset_y = (rh - lh * scale) * 0.5f;

  std::array<float, 4> c = FixColor(color);
  float lw_scaled = line_width_ * scale; 
  
  // Default style parameters
  float dash_len = 0.0f;  
  float gap_len = 0.0f;

  for (size_t i = 0; i < n - 1; ++i) {
    // Transform X
    float x1 = static_cast<float>(x[i]) * scale + offset_x;
    float x2 = static_cast<float>(x[i + 1]) * scale + offset_x;
    
    // Transform Y (Flip + Center)
    float y1 = (rh - offset_y) - static_cast<float>(y[i]) * scale;
    float y2 = (rh - offset_y) - static_cast<float>(y[i + 1]) * scale;

    lines_.push_back({x1, y1, 0.0f, x2, y2, 0.0f,
                      c[0], c[1], c[2], c[3],
                      lw_scaled, dash_len, gap_len, 0.0f, 0.0f});
  }
}

void WgpuBackend::fill(const std::vector<double>& x, const std::vector<double>& y,
                       const std::array<float, 4>& color) {
  size_t n = std::min(x.size(), y.size());
  if (n < 3) return;

  float rw = static_cast<float>(render_width_);
  float rh = static_cast<float>(render_height_);
  float lw = static_cast<float>(width_);
  float lh = static_cast<float>(height_);

  // Aspect Ratio Preserving Scale
  float scale = std::min(rw / lw, rh / lh);
  float offset_x = (rw - lw * scale) * 0.5f;
  float offset_y = (rh - lh * scale) * 0.5f;

  std::array<float, 4> c = FixFillColor(color);

  // Transform all points first
  std::vector<float> tx(n);
  std::vector<float> ty(n);
  for (size_t i = 0; i < n; ++i) {
      tx[i] = static_cast<float>(x[i]) * scale + offset_x;
      ty[i] = (rh - offset_y) - static_cast<float>(y[i]) * scale;
  }

  // Simple Triangle Fan Triangulation (Works for convex polygons like Bars/Rects)
  // v0 is pivot. Triangles: (v0, v1, v2), (v0, v2, v3), ...
  for (size_t i = 1; i < n - 1; ++i) {
      triangles_.push_back({
          tx[0], ty[0], 0.5f, 0.0f,
          tx[i], ty[i], 0.5f, 0.0f,
          tx[i+1], ty[i+1], 0.5f, 0.0f,
          0,0,1,0, 0,0,1,0, 0,0,1,0, // Normals (Z-up)
          c[0], c[1], c[2], c[3]
      });
  }
}

// fill_polygon removed

void WgpuBackend::draw_markers(const std::vector<double>& x,
                                 const std::vector<double>& y,
                                 const std::vector<double>& z) {
  size_t count = std::min(x.size(), y.size());
  (void)z;

  float rw = static_cast<float>(render_width_);
  float rh = static_cast<float>(render_height_);
  float lw = static_cast<float>(width_);
  float lh = static_cast<float>(height_);

  // Aspect Ratio Preserving Scale (same as draw_path)
  float scale = std::min(rw / lw, rh / lh);
  float offset_x = (rw - lw * scale) * 0.5f;
  float offset_y = (rh - lh * scale) * 0.5f;

  // Marker type constants (matching shader expectations)
  float marker_type = 1.0f; // Circle
  if (marker_style_ == "s") marker_type = 10.0f;      // Square
  else if (marker_style_ == "d") marker_type = 11.0f; // Diamond
  else if (marker_style_ == "+") marker_type = 12.0f; // Plus
  else if (marker_style_ == "x") marker_type = 13.0f; // Cross
  else if (marker_style_ == "^") marker_type = 14.0f; // Triangle Up
  else if (marker_style_ == "v") marker_type = 15.0f; // Triangle Down
  else if (marker_style_ == "*") marker_type = 16.0f; // Star
  else if (marker_style_ == ".") marker_type = 17.0f; // Point
  else if (marker_style_ == "p") marker_type = 16.0f; // Star (alt)

  for (size_t i = 0; i < count; ++i) {
    std::array<float, 4> c = marker_color_;
    // Apply same centering transform as draw_path
    float mx = static_cast<float>(x[i]) * scale + offset_x;
    float my = (rh - offset_y) - static_cast<float>(y[i]) * scale;
    float mz = 0.0f;
    
    WgpuRenderer::Circle circle;
    circle.cx = mx; circle.cy = my; circle.cz = mz;
    circle.radius = marker_radius_ * scale;
    circle.r = c[0]; circle.g = c[1]; circle.b = c[2]; circle.a = c[3];
    circle.type = static_cast<float>(marker_type);
    circle._p1 = 0.0f; circle._p2 = 0.0f; circle._p3 = 0.0f;
    circles_.push_back(circle);
  }
}

void WgpuBackend::draw_text(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& z) {
  if (x.empty() || y.empty() || z.empty()) return;

  std::string text_content;
  for (double val : z) {
      if (val == 0.0) break; 
      text_content += static_cast<char>(val);
  }

  if (text_content.empty()) return;

  // Apply same centering transform as draw_path
  float rw = static_cast<float>(render_width_);
  float rh = static_cast<float>(render_height_);
  float lw = static_cast<float>(width_);
  float lh = static_cast<float>(height_);
  
  float scale = std::min(rw / lw, rh / lh);
  float offset_x = (rw - lw * scale) * 0.5f;
  float offset_y = (rh - lh * scale) * 0.5f;

  // Transform coordinates from logical to render space
  float ix = static_cast<float>(x[0]);
  float iy = static_cast<float>(y[0]);
  
  float px = ix * scale + offset_x;
  float py = (rh - offset_y) - iy * scale; // Y flip + offset

  float font_size = 24.0f * scale; // Scale font too
  std::array<float, 4> color = current_text_color_;

  texts_.push_back({text_content, px, py, color[0], color[1], color[2], color[3], font_size, 0.0f});
}

void WgpuBackend::draw_image(const std::vector<std::vector<double>>& x, const std::vector<std::vector<double>>& y, const std::vector<std::vector<double>>& z) {
  if (!renderer_ || z.empty() || z[0].empty()) return;
  size_t img_height = z.size(), img_width = z[0].size();
  std::vector<float> data; data.reserve(img_width * img_height);
  double min_val = z[0][0], max_val = z[0][0];
  for (const auto& row : z) for (double val : row) { min_val = std::min(min_val, val); max_val = std::max(max_val, val); }
  double range = (max_val - min_val); if (range < 1e-9) range = 1.0;
  for (const auto& row : z) for (double val : row) data.push_back(static_cast<float>((val - min_val) / range));
  float sx = 0, sy = 0, sw = (float)width_, sh = (float)height_;
  if (!x.empty() && !x[0].empty() && x.size() >= 2) { sx = (float)x[0][0]; sw = (float)(x[0].back() - x[0][0]); }
  if (!y.empty() && !y[0].empty() && y.size() >= 2) { sy = (float)y[0][0]; sh = (float)(y.back()[0] - y[0][0]); }
  renderer_->DrawImage(data, img_width, img_height, sx, sy, sw, sh);
}

// Old draw_triangle removed

void WgpuBackend::draw_triangle_3d(const std::vector<double>& x,
                                    const std::vector<double>& y,
                                    const std::vector<double>& z,
                                    const std::array<float, 4>& color,
                                    const std::vector<double>& normals) {
  if (x.size() < 3 || y.size() < 3) return;
  std::array<float, 4> c = FixFillColor(color);
  float z1 = z.size() > 0 ? (float)z[0] : 0.5f;
  float z2 = z.size() > 1 ? (float)z[1] : 0.5f;
  float z3 = z.size() > 2 ? (float)z[2] : 0.5f;
  
  float nx1 = 0, ny1 = 0, nz1 = 1;
  float nx2 = 0, ny2 = 0, nz2 = 1;
  float nx3 = 0, ny3 = 0, nz3 = 1;
  
  if (normals.size() >= 9) {
    nx1 = (float)normals[0]; ny1 = (float)normals[1]; nz1 = (float)normals[2];
    nx2 = (float)normals[3]; ny2 = (float)normals[4]; nz2 = (float)normals[5];
    nx3 = (float)normals[6]; ny3 = (float)normals[7]; nz3 = (float)normals[8];
  } else if (normals.size() >= 3) {
    nx1 = nx2 = nx3 = (float)normals[0];
    ny1 = ny2 = ny3 = (float)normals[1];
    nz1 = nz2 = nz3 = (float)normals[2];
  }
  
  triangles_.push_back({
      (float)x[0], (float)y[0], z1, 0,
      (float)x[1], (float)y[1], z2, 0,
      (float)x[2], (float)y[2], z3, 0,
      nx1, ny1, nz1, 0,
      nx2, ny2, nz2, 0,
      nx3, ny3, nz3, 0,
      c[0], c[1], c[2], c[3]
  });
}

void WgpuBackend::ReconstructRectangles() {
  if (pending_segments_.empty()) return;
  constexpr float kTolerance = 2.0f;
  auto is_horizontal = [](const RawSegment& s) { return std::abs(s.y1 - s.y2) < kTolerance; };
  auto is_vertical = [](const RawSegment& s) { return std::abs(s.x1 - s.x2) < kTolerance; };
  auto colors_match = [](const RawSegment& a, const RawSegment& b) { return std::abs(a.r-b.r)<0.01f && std::abs(a.g-b.g)<0.01f && std::abs(a.b-b.b)<0.01f; };
  std::vector<bool> used(pending_segments_.size(), false);
  for (size_t i = 0; i < pending_segments_.size(); ++i) {
    if (used[i]) continue;
    const auto& s1 = pending_segments_[i];
    if (!is_horizontal(s1)) continue;
    float x_left = std::min(s1.x1, s1.x2), x_right = std::max(s1.x1, s1.x2), y1 = s1.y1;
    for (size_t j = i + 1; j < pending_segments_.size(); ++j) {
      if (used[j]) continue;
      const auto& s2 = pending_segments_[j];
      if (!is_horizontal(s2) || !colors_match(s1, s2) || std::abs(s2.y1 - y1) < kTolerance) continue;
      if (std::abs(std::min(s2.x1,s2.x2) - x_left) > kTolerance || std::abs(std::max(s2.x1,s2.x2) - x_right) > kTolerance) continue;
      float y2 = s2.y1;
      bool found_left = false, found_right = false; size_t left_idx = 0, right_idx = 0;
      for (size_t k = 0; k < pending_segments_.size(); ++k) {
        if (used[k] || k==i || k==j) continue;
        const auto& sv = pending_segments_[k];
        if (!is_vertical(sv) || !colors_match(s1, sv)) continue;
        if (std::abs(std::min(sv.y1,sv.y2) - std::min(y1,y2)) > kTolerance || std::abs(std::max(sv.y1,sv.y2) - std::max(y1,y2)) > kTolerance) continue;
        if (std::abs(sv.x1 - x_left) < kTolerance) { found_left = true; left_idx = k; }
        else if (std::abs(sv.x1 - x_right) < kTolerance) { found_right = true; right_idx = k; }
      }
      if (found_left && found_right) {
        rects_.push_back({x_left, std::min(y1,y2), x_right-x_left, std::max(y1,y2)-std::min(y1,y2), s1.r, s1.g, s1.b, s1.a, 0, 0, 0, 0});
        used[i] = used[j] = used[left_idx] = used[right_idx] = true; break;
      }
    }
  }
  for (size_t i = 0; i < pending_segments_.size(); ++i) {
    if (used[i]) continue;
    const auto& s = pending_segments_[i];
    lines_.push_back({s.x1, s.y1, 0, s.x2, s.y2, 0, s.r, s.g, s.b, s.a, line_width_, 0, 0, 0, 0});
  }
}

void WgpuBackend::SetViewProjection(const float* matrix) {
  if (matrix) {
      std::memcpy(view_proj_, matrix, 16 * sizeof(float));
      has_view_proj_ = true;
      if (renderer_) renderer_->SetViewProjection(matrix);
  } else {
      has_view_proj_ = false;
      if (renderer_) renderer_->SetViewProjection(nullptr);
  }
}

void WgpuBackend::set_scissor_rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h) { renderer_->SetScissorRect(x, y, w, h); }
void WgpuBackend::disable_scissor() { renderer_->DisableScissor(); }

}  // namespace matplot::backend
