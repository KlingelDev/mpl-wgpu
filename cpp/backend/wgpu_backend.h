//
// mpl-wgpu: wgpu rendering backend for matplotplusplus
// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT
//

#ifndef MPL_WGPU_BACKEND_H
#define MPL_WGPU_BACKEND_H

#include <matplot/backend/backend_interface.h>
#include <memory>
#include <vector>
#include <array>

namespace mpl_wgpu {

/// wgpu-based rendering backend for matplotplusplus
///
/// This backend provides GPU-accelerated rendering using wgpu,
/// making it suitable for high-performance visualization and
/// cross-platform deployment (including WASM).
class wgpu_backend : public matplot::backend::backend_interface {
public:
    wgpu_backend();
    explicit wgpu_backend(unsigned int width, unsigned int height);
    virtual ~wgpu_backend() override;

    // Non-copyable, non-movable (owns GPU resources)
    wgpu_backend(const wgpu_backend&) = delete;
    wgpu_backend& operator=(const wgpu_backend&) = delete;
    wgpu_backend(wgpu_backend&&) = delete;
    wgpu_backend& operator=(wgpu_backend&&) = delete;

    // Window/Output Management
    bool is_interactive() override;
    const std::string& output() override;
    const std::string& output_format() override;
    bool output(const std::string& filename) override;
    bool output(
        const std::string& filename,
        const std::string& file_format
    ) override;
    
    unsigned int width() override;
    unsigned int height() override;
    void width(unsigned int new_width) override;
    void height(unsigned int new_height) override;
    
    unsigned int position_x() override;
    unsigned int position_y() override;
    void position_x(unsigned int new_position_x) override;
    void position_y(unsigned int new_position_y) override;
    
    void window_title(const std::string& title) override;
    std::string window_title() override;

    // Frame Lifecycle
    bool new_frame() override;
    bool render_data() override;
    void show(matplot::figure_type* fig) override;
    bool should_close() override;
    bool supports_fonts() override;

    // Vertex-Based Rendering
    void draw_background(const std::array<float, 4>& color) override;
    
    void draw_rectangle(
        const double x1,
        const double x2,
        const double y1,
        const double y2,
        const std::array<float, 4>& color
    ) override;
    
    void draw_path(
        const std::vector<double>& x,
        const std::vector<double>& y,
        const std::array<float, 4>& color
    ) override;
    
    void draw_markers(
        const std::vector<double>& x,
        const std::vector<double>& y,
        const std::vector<double>& z = {}
    ) override;
    
    void draw_text(
        const std::vector<double>& x,
        const std::vector<double>& y,
        const std::vector<double>& z = {}
    ) override;
    
    void draw_triangle(
        const std::vector<double>& x,
        const std::vector<double>& y,
        const std::vector<double>& z = {}
    ) override;
    
    void draw_image(
        const std::vector<std::vector<double>>& x,
        const std::vector<std::vector<double>>& y,
        const std::vector<std::vector<double>>& z = {}
    ) override;

    // Gnuplot-specific (not used)
    bool consumes_gnuplot_commands() override;
    void run_command(const std::string& text) override;
    void include_comment(const std::string& text) override;

    // Backend-specific API
    
    /// Get raw pixel data (RGBA8)
    const uint8_t* get_pixels() const;
    
    /// Get pixel data size in bytes
    size_t get_pixels_size() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mpl_wgpu

#endif // MPL_WGPU_BACKEND_H
