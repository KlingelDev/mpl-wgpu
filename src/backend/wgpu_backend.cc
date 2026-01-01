// Copyright (c) 2025 Optris GmbH & Co. KG
//
// WebGPU-based backend for Matplot++.
// Designed for upstream contribution.

#include "matplot/backend/wgpu_backend.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>

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
  if (c[1] == 0.0f && c[2] == 0.0f && c[3] == 0.0f) {
    return {0.0f, 0.0f, 0.0f, 0.0f};
  }
  return {c[1], c[2], c[3], 1.0f};
}
}

WgpuBackend::WgpuBackend(std::shared_ptr<WgpuRenderer> renderer)
    : renderer_(std::move(renderer)) {
  spdlog::debug("WgpuBackend created");
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
  ReconstructRectangles();
  float w = static_cast<float>(render_width_);
  float h = static_cast<float>(render_height_);
  
  // Rects first (Background often included here)
  if (!rects_.empty()) renderer_->DrawRects(rects_, w, h);
  // Triangles (3D Surface)
  if (!triangles_.empty()) renderer_->DrawTriangles(triangles_, w, h);
  // Lines (Wireframe/Grid - Overlay)
  if (!lines_.empty()) renderer_->DrawLines(lines_, w, h);
  // Circles (Markers - Overlay)
  if (!circles_.empty()) renderer_->DrawCircles(circles_, w, h);
  return true;
}

bool WgpuBackend::should_close() {
  return should_close_;
}

void WgpuBackend::show(matplot::figure_type* /*fig*/) {
  render_data();
}

bool WgpuBackend::supports_fonts() {
  return false;
}

unsigned int WgpuBackend::width() { return width_; }
unsigned int WgpuBackend::height() { return height_; }
void WgpuBackend::width(unsigned int new_width) { width_ = new_width; }
void WgpuBackend::height(unsigned int new_height) { height_ = new_height; }
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
  rects_.push_back({0.0f, 0.0f, static_cast<float>(render_width_), static_cast<float>(render_height_), c[0], c[1], c[2], c[3], 0.0f, 0.0f, {0.0f, 0.0f}});
}

void WgpuBackend::draw_rectangle(double x1, double x2, double y1, double y2, const std::array<float, 4>& color) {
  float scale_x = static_cast<float>(render_width_) / static_cast<float>(width_);
  float scale_y = static_cast<float>(render_height_) / static_cast<float>(height_);
  float rh = static_cast<float>(render_height_);
  float left = static_cast<float>(std::min(x1, x2)) * scale_x;
  float right = static_cast<float>(std::max(x1, x2)) * scale_x;
  float screen_y1 = rh - static_cast<float>(std::max(y1, y2)) * scale_y;
  float screen_y2 = rh - static_cast<float>(std::min(y1, y2)) * scale_y;
  std::array<float, 4> c = FixFillColor(color);
  rects_.push_back({left, screen_y1, right - left, screen_y2 - screen_y1, c[0], c[1], c[2], c[3], 0.0f, 0.0f, {0.0f, 0.0f}});
}

// 3D Text Implementation (Matrix Projection)
// Logic: World Space Point -> Clip Space (via MVP) -> NDC (via /w) -> Screen Space
void WgpuBackend::draw_text_3d(const std::string& text, float x, float y, float z,
                           float font_size, const std::array<float, 4>& color) {
    if (!renderer_ || !has_view_proj_ || text.empty()) return;
    
    // Matrix transform (Column-Major)
    float cx = x * view_proj_[0] + y * view_proj_[4] + z * view_proj_[8] + view_proj_[12];
    float cy = x * view_proj_[1] + y * view_proj_[5] + z * view_proj_[9] + view_proj_[13];
    float cz = x * view_proj_[2] + y * view_proj_[6] + z * view_proj_[10] + view_proj_[14];
    float cw = x * view_proj_[3] + y * view_proj_[7] + z * view_proj_[11] + view_proj_[15];

    if (cw <= 0.001f) return; // Behind camera

    float ndc_x = cx / cw;
    float ndc_y = cy / cw;
    float ndc_z = cz / cw;

    // Viewport mapping
    float w = static_cast<float>(render_width_);
    float h = static_cast<float>(render_height_);
    
    float screen_x = (ndc_x + 1.0f) * 0.5f * w;
    float screen_y = (1.0f - ndc_y) * 0.5f * h;

    renderer_->DrawText(text, screen_x, screen_y, font_size, color, 0.0f);
}

void WgpuBackend::draw_path(const std::vector<double>& x, const std::vector<double>& y, const std::array<float, 4>& color, const std::string& style) {
  draw_path(x, y, color, style, {});
}

void WgpuBackend::draw_path(const std::vector<double>& x, const std::vector<double>& y, const std::array<float, 4>& color, const std::string& style, const std::vector<double>& z) {
  if (x.size() < 2 || y.size() < 2) return;
  float scale_x = static_cast<float>(render_width_) / static_cast<float>(width_);
  float scale_y = static_cast<float>(render_height_) / static_cast<float>(height_);
  float rh = static_cast<float>(render_height_);
  size_t count = std::min(x.size(), y.size());

  float dash_len = 0.0f, gap_len = 0.0f;
  if (style == "--") { dash_len = 8.0f; gap_len = 4.0f; }
  else if (style == ":") { dash_len = 2.0f; gap_len = 2.0f; }
  else if (style == "-.") { dash_len = 8.0f; gap_len = 4.0f; }

  bool is_closed_rect = false;
  if (count >= 4 && count <= 5 && z.empty()) {
    bool is_closed = (count == 5 && std::abs(x[0]-x[4]) < 0.1 && std::abs(y[0]-y[4]) < 0.1) || (count == 4 && std::abs(x[0]-x[3]) < 0.1 && std::abs(y[0]-y[3]) < 0.1);
    if (is_closed || count == 4) {
      double min_x = std::min({x[0], x[1], x[2], x[3]}), max_x = std::max({x[0], x[1], x[2], x[3]});
      double min_y = std::min({y[0], y[1], y[2], y[3]}), max_y = std::max({y[0], y[1], y[2], y[3]});
      int corners = 0;
      for (size_t i=0; i<4; ++i) if ((std::abs(x[i]-min_x)<0.1 || std::abs(x[i]-max_x)<0.1) && (std::abs(y[i]-min_y)<0.1 || std::abs(y[i]-max_y)<0.1)) corners++;
      if (corners == 4 && (max_x-min_x)>1.0 && (max_y-min_y)>1.0) {
        is_closed_rect = true;
        std::array<float, 4> c = FixFillColor(color);
        rects_.push_back({(float)min_x*scale_x, rh-(float)max_y*scale_y, (float)(max_x-min_x)*scale_x, (float)(max_y-min_y)*scale_y, c[0], c[1], c[2], c[3], 0.0f, 0.0f, {0,0}});
      }
    }
  }

  if (!is_closed_rect) {
    if (count == 2 && z.empty()) {
      pending_segments_.push_back({(float)x[0]*scale_x, rh-(float)y[0]*scale_y, (float)x[1]*scale_x, rh-(float)y[1]*scale_y, color[0], color[1], color[2], color[3]});
    } else {
      std::array<float, 4> c = FixColor(color);
      double cumulative_len = 0.0;
      float r_join = line_width_ * 0.5f;
      for (size_t i = 0; i + 1 < count; ++i) {
        float x1, y1, x2, y2, z1, z2;
        if (!z.empty()) {
          x1 = (float)x[i]; y1 = (float)y[i]; z1 = (float)z[i];
          x2 = (float)x[i+1]; y2 = (float)y[i+1]; z2 = (float)z[i+1];
        } else {
          x1 = (float)x[i]*scale_x; y1 = rh-(float)y[i]*scale_y; z1 = 0.0f;
          x2 = (float)x[i+1]*scale_x; y2 = rh-(float)y[i+1]*scale_y; z2 = 0.0f;
        }
        float seg_len = std::sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
        lines_.push_back({x1, y1, z1, x2, y2, z2, c[0], c[1], c[2], c[3], line_width_, dash_len, gap_len, (float)cumulative_len, 0.0f});
        cumulative_len += seg_len;
        if (dash_len == 0.0f) circles_.push_back({x1, y1, z1, r_join, c[0], c[1], c[2], c[3], 0.0f});
      }
      if (count > 0 && dash_len == 0.0f) {
        size_t last = count - 1;
        float lx, ly, lz = (z.size()>last)?(float)z[last]:0.0f;
        if (!z.empty()) { lx = (float)x[last]; ly = (float)y[last]; }
        else { lx = (float)x[last]*scale_x; ly = rh-(float)y[last]*scale_y; }
        circles_.push_back({lx, ly, lz, r_join, c[0], c[1], c[2], c[3], 0.0f});
      }
    }
  }
}

void WgpuBackend::draw_markers(const std::vector<double>& x, const std::vector<double>& y, const std::array<float, 4>& color, const std::vector<double>& z) {
  float scale_x = static_cast<float>(render_width_) / static_cast<float>(width_);
  float scale_y = static_cast<float>(render_height_) / static_cast<float>(height_);
  float rh = static_cast<float>(render_height_);
  size_t count = std::min(x.size(), y.size());

  using MarkerType = otc::visualization::processor::GpuProcessor::MarkerType;
  MarkerType marker_type = MarkerType::kCircle;
  if (marker_style_ == "s") marker_type = MarkerType::kSquare;
  else if (marker_style_ == "d") marker_type = MarkerType::kDiamond;
  else if (marker_style_ == "+") marker_type = MarkerType::kPlus;
  else if (marker_style_ == "x") marker_type = MarkerType::kCross;
  else if (marker_style_ == "^") marker_type = MarkerType::kTriangleUp;
  else if (marker_style_ == "v") marker_type = MarkerType::kTriangleDown;
  else if (marker_style_ == "*") marker_type = MarkerType::kStar;
  else if (marker_style_ == ".") marker_type = MarkerType::kPoint;
  else if (marker_style_ == "p") marker_type = MarkerType::kStar;

  for (size_t i = 0; i < count; ++i) {
    std::array<float, 4> c = FixColor(color);
    float mx, my, mz = (z.size()>i)?static_cast<float>(z[i]):0.0f;
    if (!z.empty()) { mx = static_cast<float>(x[i]); my = static_cast<float>(y[i]); }
    else { mx = static_cast<float>(x[i])*scale_x; my = rh-static_cast<float>(y[i])*scale_y; }
    
    WgpuRenderer::Circle circle;
    circle.cx = mx; circle.cy = my; circle.cz = mz;
    circle.radius = (float)marker_radius_;
    circle.r = c[0]; circle.g = c[1]; circle.b = c[2]; circle.a = c[3];
    circle.type = static_cast<float>(marker_type);
    circle._p1 = 0.0f; circle._p2 = 0.0f; circle._p3 = 0.0f;
    circles_.push_back(circle);
  }
}

void WgpuBackend::draw_text(const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& /*z*/) { (void)x; (void)y; }

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

void WgpuBackend::draw_triangle(const std::vector<double>& x, const std::vector<double>& y, const std::array<float, 4>& color, const std::vector<double>& z) {
  if (x.size() < 3 || y.size() < 3) return;
  float scale_x = (float)render_width_/(float)width_, scale_y = (float)render_height_/(float)height_, rh = (float)render_height_;
  std::array<float, 4> c = FixFillColor(color);
  float z1 = z.size()>0?(float)z[0]:0.5f, z2 = z.size()>1?(float)z[1]:0.5f, z3 = z.size()>2?(float)z[2]:0.5f;
  float tx1, ty1, tx2, ty2, tx3, ty3;
  if (!z.empty()) {
    tx1 = (float)x[0]; ty1 = (float)y[0];
    tx2 = (float)x[1]; ty2 = (float)y[1];
    tx3 = (float)x[2]; ty3 = (float)y[2];
  } else {
    tx1 = (float)x[0]*scale_x; ty1 = rh-(float)y[0]*scale_y;
    tx2 = (float)x[1]*scale_x; ty2 = rh-(float)y[1]*scale_y;
    tx3 = (float)x[2]*scale_x; ty3 = rh-(float)y[2]*scale_y;
  }
  triangles_.push_back({tx1, ty1, z1, 0, tx2, ty2, z2, 0, tx3, ty3, z3, 0, 0,0,1,0, 0,0,1,0, 0,0,1,0, c[0], c[1], c[2], c[3]});
}

void WgpuBackend::draw_triangle(const std::vector<double>& x, const std::vector<double>& y, const std::array<float, 4>& color, const std::vector<double>& z, const std::vector<double>& normals) {
  if (x.size() < 3 || y.size() < 3) return;
  std::array<float, 4> c = color;
  float z1 = z.size()>0?(float)z[0]:0.5f, z2 = z.size()>1?(float)z[1]:0.5f, z3 = z.size()>2?(float)z[2]:0.5f;
  float nx1=0, ny1=0, nz1=1, nx2=0, ny2=0, nz2=1, nx3=0, ny3=0, nz3=1;
  if (normals.size()>=9) {
      nx1=(float)normals[0]; ny1=(float)normals[1]; nz1=(float)normals[2];
      nx2=(float)normals[3]; ny2=(float)normals[4]; nz2=(float)normals[5];
      nx3=(float)normals[6]; ny3=(float)normals[7]; nz3=(float)normals[8];
  } else if (normals.size()>=3) {
      nx1=nx2=nx3=(float)normals[0]; ny1=ny2=ny3=(float)normals[1]; nz1=nz2=nz3=(float)normals[2];
  }
  triangles_.push_back({(float)x[0], (float)y[0], z1, 0, (float)x[1], (float)y[1], z2, 0, (float)x[2], (float)y[2], z3, 0, nx1, ny1, nz1, 0, nx2, ny2, nz2, 0, nx3, ny3, nz3, 0, c[0], c[1], c[2], c[3]});
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
        rects_.push_back({x_left, std::min(y1,y2), x_right-x_left, std::max(y1,y2)-std::min(y1,y2), s1.r, s1.g, s1.b, s1.a, 0, 0, {0,0}});
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
