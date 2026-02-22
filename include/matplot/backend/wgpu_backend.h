// SPDX-License-Identifier: MIT
//
// WebGPU backend for matplot++ plotting library
// This file is designed to be contributed upstream to matplotplusplus.

#ifndef MATPLOT_BACKEND_WGPU_BACKEND_H_
#define MATPLOT_BACKEND_WGPU_BACKEND_H_

#include <matplot/backend/backend_interface.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace matplot::backend {

/// @brief Abstract rendering interface for WebGPU drawing operations.
///
/// Implement this interface to provide the actual GPU draw calls.
/// This allows the WgpuBackend to be independent of any specific
/// rendering infrastructure.
class WgpuRenderer {
 public:
  virtual ~WgpuRenderer() = default;

  /// @brief Sets the View-Projection matrix (MVP) for 3D rendering.
  virtual void SetViewProjection(const float* matrix) {}

  /// @brief Sets the scissor rect for clipping.
  virtual void SetScissorRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {}

  /// @brief Disables the scissor rect.
  virtual void DisableScissor() {}

  // =========================================================================
  // Primitive Data Structures (16-byte aligned for GPU)
  // =========================================================================

  /// @brief Rectangle instance data for batched rendering.
  struct Rect {
    float x, y, width, height;
    float r, g, b, a;
    float stroke_width;
    float corner_radius;
    float z;
    float padding; // Reduced padding
  };

  /// @brief Line instance data for batched rendering.
  struct Line {
    float x1, y1, z1, x2, y2, z2;
    float r, g, b, a;
    float width;
    float dash_len;    // Length of dash in pixels (0 for solid)
    float gap_len;     // Length of gap in pixels
    float dash_offset; // Starting offset
    float padding; // Alignment
  };

  /// @brief Circle/marker instance data for batched rendering.
  struct Circle {
    float cx, cy, cz, radius;
    float r, g, b, a;
    float type;
    float stroke_width;
    float _p2, _p3;
  };

  /// @brief Triangle instance data for batched rendering.
  struct Triangle {
      float x1, y1, z1, _p1;
      float x2, y2, z2, _p2;
      float x3, y3, z3, _p3;
      float nx1, ny1, nz1, _p4;
      float nx2, ny2, nz2, _p5;
      float nx3, ny3, nz3, _p6;
      float r, g, b, a;
  };

  /// @brief Text instance data for batched rendering.
  struct TextCommand {
    std::string text;
    float x, y;
    float r, g, b, a;
    float font_size;
    float rotation;
  };

  // =========================================================================
  // Rendering Interface
  // =========================================================================

  /// @brief Draws a batch of rectangles.
  virtual void DrawRects(const std::vector<Rect>& rects,
                         float screen_width, float screen_height) = 0;

  /// @brief Draws a batch of lines.
  virtual void DrawLines(const std::vector<Line>& lines,
                         float screen_width, float screen_height) = 0;

  /// @brief Draws a batch of circles (point markers).
  virtual void DrawCircles(const std::vector<Circle>& circles,
                           float screen_width, float screen_height) = 0;

  /// @brief Draws a batch of triangles.
  virtual void DrawTriangles(const std::vector<Triangle>& triangles,
                             float screen_width, float screen_height) = 0;

  /// @brief Draws text at a specific location.
  virtual void DrawText(const std::string& text, float x, float y,
                        float font_size, const std::array<float, 4>& color,
                        float rotation = 0.0f) = 0;

  virtual float MeasureText(const std::string& text, float font_size) = 0;

  /// @brief Clears the current frame.
  virtual void Clear(const std::array<float, 4>& color) = 0;

  /// @brief Draws an image/heatmap at the specified rectangle.
  /// @param data Grayscale values (0.0 to 1.0 normalized).
  /// @param img_width Image width in pixels.
  /// @param img_height Image height in pixels.
  /// @param x Screen X position (left).
  /// @param y Screen Y position (top).
  /// @param width Display width on screen.
  /// @param height Display height on screen.
  virtual void DrawImage(const std::vector<float>& data,
                         size_t img_width, size_t img_height,
                         float x, float y, float width, float height) = 0;
};

/// @brief WebGPU backend for matplot++ that uses a WgpuRenderer for drawing.
///
/// This backend collects drawing primitives (paths, rectangles, markers)
/// during a frame and flushes them via the provided renderer when
/// render_data() is called.
///
/// Design notes for upstream contribution:
/// - Self-contained with minimal dependencies (only matplot++ and webgpu.hpp)
/// - Uses abstract WgpuRenderer interface for actual GPU operations
/// - Follows the pattern of matplotlib's Agg backend
///
/// Example usage:
/// @code
///   class MyRenderer : public WgpuRenderer { /* ... */ };
///   auto renderer = std::make_shared<MyRenderer>();
///   auto backend = std::make_shared<WgpuBackend>(renderer);
///   auto fig = matplot::figure();
///   fig->backend(backend);
///   matplot::plot({1, 2, 3}, {1, 4, 9});
///   fig->draw();
/// @endcode
class WgpuBackend : public backend_interface {
 public:
  /// @brief Constructs a backend with the given renderer.
  /// @param renderer Shared pointer to a WgpuRenderer implementation.
  explicit WgpuBackend(std::shared_ptr<WgpuRenderer> renderer);

  ~WgpuBackend() = default;

  // =========================================================================
  // Lifecycle Methods
  // =========================================================================

  /// @brief Returns true since this is an interactive (window) backend.
  bool is_interactive() override;

  /// @brief Prepares for a new frame, clearing batched draw data.
  bool new_frame() override;

  /// @brief Flushes all batched draw calls to the renderer.
  bool render_data() override;

  /// @brief Returns true if the window was closed.
  bool should_close() override;

  /// @brief Show the figure (blocks until closed for interactive backends).
  void show(matplot::figure_type* fig) override;

  /// @brief Returns true (we support font rendering via renderer).
  bool supports_fonts() override;

  // =========================================================================
  // Dimension Methods
  // =========================================================================

  unsigned int width() override;
  unsigned int height() override;
  void width(unsigned int new_width) override;
  void height(unsigned int new_height) override;

  unsigned int position_x() override;
  unsigned int position_y() override;
  void position_x(unsigned int x) override;
  void position_y(unsigned int y) override;

  // =========================================================================
  // Drawing Methods (Primitive Rendering)
  // =========================================================================

  /// @brief Fills the background with a solid color.
  void draw_background(const std::array<float, 4>& color) override;

  /// @brief Draws a rectangle outline or fill.
  void draw_rectangle(double x1, double x2, double y1, double y2,
                      const std::array<float, 4>& color) override;

  /// @brief Draws a path (polyline) as connected line segments.
  void draw_path(const std::vector<double>& x, const std::vector<double>& y,
                 const std::array<float, 4>& color) override;

  /// @brief Fills a polygon.
  void fill(const std::vector<double>& x, const std::vector<double>& y,
            const std::array<float, 4>& color);

  /// @brief Draws point markers at the given coordinates.
  void draw_markers(const std::vector<double>& x,
                    const std::vector<double>& y,
                    const std::vector<double>& z = {}) override;

  // Configuration methods (not virtual in base class).
  void marker_size(float size) { set_marker_radius(size); }
  void marker_style(const std::string& style) { marker_style_ = style; }

  /// @brief Draws text at the given coordinates.
  void draw_text(const std::vector<double>& x, const std::vector<double>& y,
                 const std::vector<double>& z = {}) override;

  /// @brief Draws a text label at a given position.
  void draw_label(const std::string& text, double x, double y,
                  float font_size,
                  const std::array<float, 4>& color) override;

  void draw_label(const std::string& text, double x, double y,
                  float font_size,
                  const std::array<float, 4>& color,
                  float rotation) override;

  /// @brief Measure text width in viewport units.
  double text_width(const std::string& text,
                    float font_size) override;

  /// \brief Set text color (not virtual in base class).
  void text_color(const std::array<float, 4> &color) {
      current_text_color_ = color;
  }

  /// @brief Draws an image/heatmap.
  void draw_image(const std::vector<std::vector<double>>& x,
                  const std::vector<std::vector<double>>& y,
                  const std::vector<std::vector<double>>& z = {}) override;

  /// @brief Draws triangles.
  void draw_triangle(const std::vector<double>& x,
                     const std::vector<double>& y,
                     const std::vector<double>& z = {}) override;

  // Overload for 3D with normals
  void draw_triangle_3d(const std::vector<double>& x,
                       const std::vector<double>& y,
                       const std::vector<double>& z,
                       const std::array<float, 4>& color,
                       const std::vector<double>& normals);

  // 3D Text Helper (Uses stored MVP matrix)
  void draw_text_3d(const std::string& text, float x, float y, float z,
                    float font_size, const std::array<float, 4>& color);

  // =========================================================================
  // Configuration (not part of backend_interface, extensions)
  // =========================================================================

  /// @brief Sets the line/stroke width for paths.
  void set_line_width(float width) override {
    line_width_ = width;
  }

  /// @brief Gets the current line width.
  float line_width() const { return line_width_; }

  /// @brief Sets the marker radius for draw_markers.
  void set_marker_radius(float radius) override {
    marker_radius_ = radius;
  }

  /// @brief Gets the current marker radius.
  float marker_radius() const override { return marker_radius_; }

  /// @brief Sets whether markers are filled or hollow.
  void set_marker_face(bool face) override {
    marker_face_ = face;
  }

  /// @brief Sets the default marker color.
  void set_marker_color(
      const std::array<float, 4>& color) override {
    marker_color_ = color;
  }

  /// @brief Sets the marker style (e.g., 'o', 's', '+').
  const std::string& marker_style() const { return marker_style_; }

  /// @brief Sets the actual render target size (independent of matplot++).
  /// Call this when the window resizes to ensure correct coordinate mapping.
  void set_render_size(unsigned int w, unsigned int h);

  /// @brief Sets the View-Projection matrix for 3D logic.
  void SetViewProjection(const float* matrix);

   void set_scissor_rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h);
 
   /// @brief Disables the scissor rect.
   void disable_scissor();

  /// @brief Override to indicate this backend doesn't support gnuplot
  /// This prevents matplot++ from trying to send gnuplot commands
  void run_command(const std::string&) override {}
 
  protected:
  std::shared_ptr<WgpuRenderer> renderer_;
  unsigned int width_{0};
  unsigned int height_{0};
    
  // 3D Matrix Storage (Column-Major)
  float view_proj_[16] = {0};
  bool has_view_proj_ = false;

  // Buffers for batching
  unsigned int pos_x_ = 100;
  unsigned int pos_y_ = 100;
  bool should_close_ = false;

  std::array<float, 4> current_text_color_ = {0.0f, 0.0f, 0.0f, 1.0f}; // Default Black
  
  float line_width_ = 1.0f;    // Default line width (pixels)
  float marker_radius_ = 6.0f; // Default marker radius (pixels)
  std::array<float, 4> marker_color_ = {1.0f, 0.0f, 0.0f, 1.0f};
  std::string marker_style_ = "o"; // Default marker style (circle)
  bool marker_face_ = true;    // Filled (true) or hollow (false)

  // Batched draw data collected during frame
  std::vector<WgpuRenderer::Rect> rects_;
  std::vector<WgpuRenderer::Line> lines_;
  std::vector<WgpuRenderer::Circle> circles_;
  std::vector<WgpuRenderer::Triangle> triangles_;
  std::vector<WgpuRenderer::TextCommand> texts_;


  // Raw segment data for rectangle reconstruction
  // Used to detect 4-segment rectangles (bars) and fill them
  struct RawSegment {
    float x1, y1, x2, y2;
    float r, g, b, a;
  };
  std::vector<RawSegment> pending_segments_;

  // Actual render target size (not modified by matplot++)
  unsigned int render_width_ = 800;
  unsigned int render_height_ = 600;

  // Helper: Reconstruct rectangles from pending segments
  void ReconstructRectangles();
};

}  // namespace matplot::backend

#endif  // MATPLOT_BACKEND_WGPU_BACKEND_H_
