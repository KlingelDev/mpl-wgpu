//
// mpl-wgpu: wgpu rendering backend for matplotplusplus
// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT
//

#include "wgpu_backend.h"
#include <stdexcept>

namespace mpl_wgpu {

// Forward declaration for PIMPL
struct wgpu_backend::Impl {
    unsigned int width_ = 800;
    unsigned int height_ = 600;
    unsigned int position_x_ = 0;
    unsigned int position_y_ = 0;
    std::string output_file_;
    std::string output_format_;
    std::string window_title_ = "mpl-wgpu";
    bool should_close_ = false;
    std::vector<uint8_t> pixel_buffer_;

    // TODO: Add wgpu device, queue, texture, etc.
    // void* wgpu_device = nullptr;
    // void* wgpu_queue = nullptr;
    // void* render_texture = nullptr;
    
    Impl(unsigned int w, unsigned int h)
        : width_(w), height_(h) {
        pixel_buffer_.resize(w * h * 4, 0); // RGBA
    }
};

wgpu_backend::wgpu_backend()
    : impl_(std::make_unique<Impl>(800, 600)) {
}

wgpu_backend::wgpu_backend(unsigned int width, unsigned int height)
    : impl_(std::make_unique<Impl>(width, height)) {
}

wgpu_backend::~wgpu_backend() = default;

// Window/Output Management

bool wgpu_backend::is_interactive() {
    // For now, non-interactive (offscreen rendering)
    return false;
}

const std::string& wgpu_backend::output() {
    return impl_->output_file_;
}

const std::string& wgpu_backend::output_format() {
    return impl_->output_format_;
}

bool wgpu_backend::output(const std::string& filename) {
    impl_->output_file_ = filename;
    // Auto-detect format from extension
    size_t dot = filename.find_last_of('.');
    if (dot != std::string::npos) {
        impl_->output_format_ = filename.substr(dot + 1);
    }
    return true;
}

bool wgpu_backend::output(
    const std::string& filename,
    const std::string& file_format
) {
    impl_->output_file_ = filename;
    impl_->output_format_ = file_format;
    return true;
}

unsigned int wgpu_backend::width() {
    return impl_->width_;
}

unsigned int wgpu_backend::height() {
    return impl_->height_;
}

void wgpu_backend::width(unsigned int new_width) {
    impl_->width_ = new_width;
    impl_->pixel_buffer_.resize(new_width * impl_->height_ * 4);
}

void wgpu_backend::height(unsigned int new_height) {
    impl_->height_ = new_height;
    impl_->pixel_buffer_.resize(impl_->width_ * new_height * 4);
}

unsigned int wgpu_backend::position_x() {
    return impl_->position_x_;
}

unsigned int wgpu_backend::position_y() {
    return impl_->position_y_;
}

void wgpu_backend::position_x(unsigned int new_position_x) {
    impl_->position_x_ = new_position_x;
}

void wgpu_backend::position_y(unsigned int new_position_y) {
    impl_->position_y_ = new_position_y;
}

void wgpu_backend::window_title(const std::string& title) {
    impl_->window_title_ = title;
}

std::string wgpu_backend::window_title() {
    return impl_->window_title_;
}

// Frame Lifecycle

bool wgpu_backend::new_frame() {
    // TODO: Begin wgpu render pass
    return !impl_->should_close_;
}

bool wgpu_backend::render_data() {
    // TODO: End wgpu render pass and present
    return true;
}

void wgpu_backend::show(matplot::figure_type* fig) {
    // For non-interactive backend, this is a no-op
    (void)fig;
}

bool wgpu_backend::should_close() {
    return impl_->should_close_;
}

bool wgpu_backend::supports_fonts() {
    // TextRenderer supports fonts
    return true;
}

// Vertex-Based Rendering

void wgpu_backend::draw_background(const std::array<float, 4>& color) {
    // TODO: Clear render texture with color
    // For now, just fill pixel buffer
    uint8_t r = static_cast<uint8_t>(color[0] * 255);
    uint8_t g = static_cast<uint8_t>(color[1] * 255);
    uint8_t b = static_cast<uint8_t>(color[2] * 255);
    uint8_t a = static_cast<uint8_t>(color[3] * 255);
    
    for (size_t i = 0; i < impl_->pixel_buffer_.size(); i += 4) {
        impl_->pixel_buffer_[i + 0] = r;
        impl_->pixel_buffer_[i + 1] = g;
        impl_->pixel_buffer_[i + 2] = b;
        impl_->pixel_buffer_[i + 3] = a;
    }
}

void wgpu_backend::draw_rectangle(
    const double x1,
    const double x2,
    const double y1,
    const double y2,
    const std::array<float, 4>& color
) {
    // TODO: Use PrimitiveRenderer to draw filled rectangle
    (void)x1; (void)x2; (void)y1; (void)y2; (void)color;
}

void wgpu_backend::draw_path(
    const std::vector<double>& x,
    const std::vector<double>& y,
    const std::array<float, 4>& color
) {
    // TODO: Use PrimitiveRenderer to draw line strip
    (void)x; (void)y; (void)color;
}

void wgpu_backend::draw_markers(
    const std::vector<double>& x,
    const std::vector<double>& y,
    const std::vector<double>& z
) {
    // TODO: Use PrimitiveRenderer to draw markers
    (void)x; (void)y; (void)z;
}

void wgpu_backend::draw_text(
    const std::vector<double>& x,
    const std::vector<double>& y,
    const std::vector<double>& z
) {
    // TODO: Use TextRenderer to draw text
    (void)x; (void)y; (void)z;
}

void wgpu_backend::draw_triangle(
    const std::vector<double>& x,
    const std::vector<double>& y,
    const std::vector<double>& z
) {
    // TODO: Render triangle primitive
    (void)x; (void)y; (void)z;
}

void wgpu_backend::draw_image(
    const std::vector<std::vector<double>>& x,
    const std::vector<std::vector<double>>& y,
    const std::vector<std::vector<double>>& z
) {
    // TODO: Render 2D image/texture
    (void)x; (void)y; (void)z;
}

// Gnuplot-specific (not used)

bool wgpu_backend::consumes_gnuplot_commands() {
    return false; // We are a vertex-based backend
}

void wgpu_backend::run_command(const std::string& text) {
    (void)text; // No-op
}

void wgpu_backend::include_comment(const std::string& text) {
    (void)text; // No-op
}

// Backend-specific API

const uint8_t* wgpu_backend::get_pixels() const {
    return impl_->pixel_buffer_.data();
}

size_t wgpu_backend::get_pixels_size() const {
    return impl_->pixel_buffer_.size();
}

} // namespace mpl_wgpu
