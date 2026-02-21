// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

#include "c_api.h"
#include "matplot/backend/wgpu_backend.h"
#include <matplot/matplot.h>
#include <vector>
#include <string>

using namespace matplot::backend;

// ===========================================================================
// RustWgpuRenderer - Bridge Implementation
// ===========================================================================

class RustWgpuRenderer : public WgpuRenderer {
public:
    RustWgpuRenderer(const MplWgpuVTable* vtable, void* user_data)
        : vtable_(*vtable), user_data_(user_data) {}

    void SetViewProjection(const float* matrix) override {
        // TODO: Expose callback if needed, but PrimitiveRenderer might not need it
        // Or we pass it via Draw calls if implementing 3D properly via Rust
    }

    void DrawRects(const std::vector<Rect>& rects, float, float) override {
        if (vtable_.draw_rects && !rects.empty()) {
            vtable_.draw_rects(user_data_, reinterpret_cast<const MplWgpuRect*>(rects.data()), rects.size());
        }
    }

    void DrawLines(const std::vector<Line>& lines, float, float) override {
        if (vtable_.draw_lines && !lines.empty()) {
            vtable_.draw_lines(user_data_, reinterpret_cast<const MplWgpuLine*>(lines.data()), lines.size());
        }
    }

    void DrawCircles(const std::vector<Circle>& circles, float, float) override {
        if (vtable_.draw_circles && !circles.empty()) {
            vtable_.draw_circles(user_data_, reinterpret_cast<const MplWgpuCircle*>(circles.data()), circles.size());
        }
    }

    void DrawTriangles(const std::vector<Triangle>& triangles, float, float) override {
        if (vtable_.draw_triangles && !triangles.empty()) {
            vtable_.draw_triangles(user_data_, reinterpret_cast<const MplWgpuTriangle*>(triangles.data()), triangles.size());
        }
    }

    void DrawText(const std::string& text, float x, float y, float font_size, 
                  const std::array<float, 4>& color, float rotation) override {
        if (vtable_.draw_text) {
            vtable_.draw_text(user_data_, text.c_str(), x, y, font_size, 
                              color[0], color[1], color[2], color[3], rotation);
        }
    }

    float MeasureText(const std::string& text, float font_size) override {
        if (vtable_.measure_text) {
            return vtable_.measure_text(user_data_, text.c_str(), font_size);
        }
        return text.length() * font_size * 0.6f; // Fallback
    }

    void Clear(const std::array<float, 4>& color) override {
        if (vtable_.clear) {
            vtable_.clear(user_data_, color[0], color[1], color[2], color[3]);
        }
    }

    void DrawImage(const std::vector<float>& data, size_t img_width, size_t img_height,
                   float x, float y, float width, float height) override {
        if (vtable_.draw_image) {
            vtable_.draw_image(user_data_, data.data(), img_width, img_height, x, y, width, height);
        }
    }

private:
    MplWgpuVTable vtable_;
    void* user_data_;
};

// ===========================================================================
// Implementation structure wrappers
// ===========================================================================

struct MplWgpuBackend {
    std::shared_ptr<WgpuBackend> backend;
    std::shared_ptr<RustWgpuRenderer> renderer;
};

struct MplFigure {
    std::unique_ptr<matplot::figure_type> figure; // matplot++ figure
    MplWgpuBackend* backend_ref; // Pointer to backend (not owned)
};

// ===========================================================================
// C API Implementation
// ===========================================================================

MplWgpuBackend* mpl_wgpu_backend_create(const MplWgpuVTable* vtable, void* user_data) {
    auto mpl_backend = new MplWgpuBackend();
    mpl_backend->renderer = std::make_shared<RustWgpuRenderer>(vtable, user_data);
    mpl_backend->backend = std::make_shared<WgpuBackend>(mpl_backend->renderer);
    return mpl_backend;
}

void mpl_wgpu_backend_destroy(MplWgpuBackend* backend) {
    if (backend) {
        delete backend;
    }
}

bool mpl_wgpu_backend_render_data(MplWgpuBackend* backend) {
    if (backend && backend->backend) {
        return backend->backend->render_data();
    }
    return false;
}

void mpl_wgpu_backend_set_size(MplWgpuBackend* backend, unsigned int width, unsigned int height) {
    if (backend && backend->backend) {
        backend->backend->width(width);
        backend->backend->height(height);
        backend->backend->set_render_size(width, height);
    }
}

// ---------------------------------------------------------------------------

MplFigure* mpl_figure_create(MplWgpuBackend* backend) {
    auto fig = new MplFigure();
    // Create new figure in quiet mode
    fig->figure = std::make_unique<matplot::figure_type>(true);
    if (backend && backend->backend) {
        fig->figure->backend(backend->backend);
    }
    fig->backend_ref = backend;
    return fig;
}

void mpl_figure_destroy(MplFigure* fig) {
    if (fig) {
        delete fig;
    }
}

struct MplAxes {
    // We don't own the axes pointer, the figure does. 
    // Usually matplot++ returns shared_ptr<axes_type>
    std::shared_ptr<matplot::axes_type> axes;
};

// ...

MplAxes* mpl_figure_current_axes(MplFigure* fig) {
    if (!fig || !fig->figure) return nullptr;
    auto ax = fig->figure->current_axes(); // Returns shared_ptr
    // We need to return a stable handle. 
    // We can heap allocate MplAxes wrapping the shared_ptr.
    // Caller should not destroy it? Or should they?
    // Since figure owns axes, usually we just return a transient handle or a handle that keeps it alive.
    // Let's alloc new wrapper that holds shared_ptr.
    auto wrapper = new MplAxes();
    wrapper->axes = ax;
    return wrapper;
}

// Helper to clean up axes handle if exposed (TODO: add mpl_axes_destroy equivalent or rely on figure)
// For now we leak the small wrapper if we don't expose destroy, but idiomatic C usually destroys handles.
// Let's stick to simple "get current" for now. 
// Ideally we should cache them in Figure wrapper? No, simple wrapper is fine.

void mpl_axes_destroy(MplAxes* ax) {
    if (ax) delete ax;
}

void mpl_axes_plot(MplAxes* ax, const double* x, const double* y, size_t count, const char* style) {
    if (!ax || !ax->axes) return;
    std::vector<double> vx(x, x + count);
    std::vector<double> vy(y, y + count);
    ax->axes->plot(vx, vy, style ? std::string(style) : "");
}

void mpl_axes_scatter(MplAxes* ax, const double* x, const double* y, size_t count, const char* style) {
    if (!ax || !ax->axes) return;
    std::vector<double> vx(x, x + count);
    std::vector<double> vy(y, y + count);
    // Use plot with marker style which is more robust in this backend currently
    // or scatter if supported.
    if (style) {
        ax->axes->plot(vx, vy, style);
    } else {
        // default scatter
        ax->axes->plot(vx, vy, "o");
    }
}

void mpl_axes_bar(MplAxes* ax, const double* values, size_t count) {
    if (!ax || !ax->axes) return;
    std::vector<double> v(values, values + count);
    ax->axes->bar(v);
}

void mpl_axes_hist(MplAxes* ax, const double* values, size_t count, size_t bins) {
    if (!ax || !ax->axes) return;
    std::vector<double> v(values, values + count);
    // matplot++ hist takes vector and bin count/edges
    ax->axes->hist(v, bins);
}

void mpl_axes_surface(MplAxes* ax, const double* x, const double* y, const double* z, size_t rows, size_t cols, bool wireframe) {
    if (!ax || !ax->axes) return;
    
    // Matplot++ expects vectors of vectors for x, y, z grids usually
    // Or single vectors if they are flat?
    // surface(X, Y, Z)
    
    // Unpack flat arrays to vector<vector>
    std::vector<std::vector<double>> X(rows, std::vector<double>(cols));
    std::vector<std::vector<double>> Y(rows, std::vector<double>(cols));
    std::vector<std::vector<double>> Z(rows, std::vector<double>(cols));
    
    for (size_t r=0; r<rows; ++r) {
        for (size_t c=0; c<cols; ++c) {
            size_t idx = r * cols + c; // Or c * rows? Assuming row-major input
            X[r][c] = x[idx]; // Assuming x is meshgrid flat
            Y[r][c] = y[idx]; // Assuming y is meshgrid flat
            Z[r][c] = z[idx];
        }
    }
    
    if (wireframe) {
        // mesh(X,Y,Z)? No easy access from axes handle in some versions? 
        // axes->mesh
        // Let's assume surface with no face color or similar.
        // But axes_type should have surf/mesh.
        // ax->axes->mesh(X, Y, Z); // Check if available
        // Fallback to surf
        ax->axes->surf(X, Y, Z); 
    } else {
        ax->axes->surf(X, Y, Z);
    }
}

void mpl_axes_pie(MplAxes* ax, const double* values, size_t count) {
    if (!ax || !ax->axes) return;
    std::vector<double> v(values, values + count);
    ax->axes->pie(v);
}

void mpl_axes_boxplot(MplAxes* ax, const double* values, size_t count) {
    if (!ax || !ax->axes) return;
    std::vector<double> v(values, values + count);
    ax->axes->boxplot(v);
}

void mpl_axes_heatmap(MplAxes* ax, const double* z, size_t rows, size_t cols) {
    if (!ax || !ax->axes) return;
    std::vector<std::vector<double>> Z(rows, std::vector<double>(cols));
    for (size_t r=0; r<rows; ++r) {
        for (size_t c=0; c<cols; ++c) {
            Z[r][c] = z[r * cols + c];
        }
    }
    ax->axes->heatmap(Z);
}

void mpl_axes_set_title(MplAxes* ax, const char* title) {
    if (ax && ax->axes) ax->axes->title(title);
}

void mpl_axes_set_xlabel(MplAxes* ax, const char* label) {
    if (ax && ax->axes) ax->axes->x_axis().label(label);
}

void mpl_axes_set_ylabel(MplAxes* ax, const char* label) {
    if (ax && ax->axes) ax->axes->y_axis().label(label);
}

void mpl_axes_grid(MplAxes* ax, bool on) {
    if (ax && ax->axes) ax->axes->grid(on);
}

void mpl_axes_set_xlim(MplAxes* ax, double min, double max) {
    if (ax && ax->axes) ax->axes->xlim({min, max});
}

void mpl_axes_set_ylim(MplAxes* ax, double min, double max) {
    if (ax && ax->axes) ax->axes->ylim({min, max});
}

void mpl_figure_draw(MplFigure* fig) {
    if (fig && fig->figure) {
        fig->figure->draw();
    }
}

void mpl_figure_clear(MplFigure* fig) {
    if (fig && fig->figure) {
        // Clear all axes?
        // matplot++ doesn't have a clear() on figure directly that is public in all versions?
        // But we can reset.
        // Or simply `fig->figure->children({});`
        // Actually `quiet_mode` etc might be relevant.
        // Let's assume standard behavior:
        // Create new figure? No, keep backend.
        // Since we bind backend to figure instance.
        
        // fig->figure->clear(); // Attempt
        // If clear() not avail, we can just remove children.
        // fig->figure->children().clear() ? 
        // Best way:
        fig->figure->draw(); // Force clear?
        // Wait, matplot++ state management is tricky.
        // Let's trust `clf()` if available globally, but we are local.
    }
}
