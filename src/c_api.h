// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===========================================================================
// Opaque types
// ===========================================================================
typedef struct MplWgpuBackend MplWgpuBackend;
typedef struct MplFigure MplFigure;
typedef struct MplAxes MplAxes;

// Axis / Figure management
MplFigure* mpl_figure_create(MplWgpuBackend* backend);
MplFigure* mpl_figure_create_gnuplot(void);
void mpl_figure_destroy(MplFigure* fig);
MplAxes* mpl_figure_current_axes(MplFigure* fig);
void mpl_axes_destroy(MplAxes* ax);
bool mpl_figure_save(MplFigure* fig, const char* path);

// Plotting commands (on Axes)
void mpl_axes_plot(MplAxes* ax, const double* x, const double* y, size_t count, const char* style);
void mpl_axes_scatter(MplAxes* ax, const double* x, const double* y, size_t count, const char* style);
void mpl_axes_bar(MplAxes* ax, const double* values, size_t count); // Simplified for now
void mpl_axes_hist(MplAxes* ax, const double* values, size_t count, size_t bins);
void mpl_axes_surface(MplAxes* ax, const double* x, const double* y, const double* z, size_t rows, size_t cols, bool wireframe);
void mpl_axes_pie(MplAxes* ax, const double* values, size_t count);
void mpl_axes_boxplot(MplAxes* ax, const double* values, size_t count); // Simplified 1D
void mpl_axes_heatmap(MplAxes* ax, const double* z, size_t rows, size_t cols);

void mpl_axes_set_title(MplAxes* ax, const char* title);
void mpl_axes_set_xlabel(MplAxes* ax, const char* label);
void mpl_axes_set_ylabel(MplAxes* ax, const char* label);
void mpl_axes_grid(MplAxes* ax, bool on);
void mpl_axes_set_xlim(MplAxes* ax, double min, double max);
void mpl_axes_set_ylim(MplAxes* ax, double min, double max);

// Figure level
void mpl_figure_draw(MplFigure* fig);
void mpl_figure_clear(MplFigure* fig);
// ===========================================================================

typedef struct {
    float x, y, width, height;
    float r, g, b, a;
    float stroke_width;
    float corner_radius;
    float z;
    float padding;
} MplWgpuRect;

typedef struct {
    float x1, y1, z1, x2, y2, z2;
    float r, g, b, a;
    float width;
    float dash_len;
    float gap_len;
    float dash_offset;
    float padding;
} MplWgpuLine;

typedef struct {
    float cx, cy, cz, radius;
    float r, g, b, a;
    float type;      // Marker type
    float _p1, _p2, _p3; // Padding
} MplWgpuCircle;

typedef struct {
    float x1, y1, z1, _p1;
    float x2, y2, z2, _p2;
    float x3, y3, z3, _p3;
    float nx1, ny1, nz1, _p4;
    float nx2, ny2, nz2, _p5;
    float nx3, ny3, nz3, _p6;
    float r, g, b, a;
} MplWgpuTriangle;

// Note: String passed separately
typedef struct {
    float x, y;
    float r, g, b, a;
    float font_size;
    float rotation;
    // float padding; // Adjust if needed
} MplWgpuTextData;

// ===========================================================================
// Callbacks (VTable)
// ===========================================================================

typedef void (*MplWgpuDrawRectsFn)(void* user_data, const MplWgpuRect* rects, size_t count);
typedef void (*MplWgpuDrawLinesFn)(void* user_data, const MplWgpuLine* lines, size_t count);
typedef void (*MplWgpuDrawCirclesFn)(void* user_data, const MplWgpuCircle* circles, size_t count);
typedef void (*MplWgpuDrawTrianglesFn)(void* user_data, const MplWgpuTriangle* triangles, size_t count);
typedef void (*MplWgpuDrawTextFn)(void* user_data, const char* text, float x, float y, float font_size, 
                                  float r, float g, float b, float a, float rotation);
typedef float (*MplWgpuMeasureTextFn)(void* user_data, const char* text, float font_size);
typedef void (*MplWgpuClearFn)(void* user_data, float r, float g, float b, float a);
typedef void (*MplWgpuDrawImageFn)(void* user_data, const float* data, size_t img_width, size_t img_height, 
                                   float x, float y, float width, float height);

typedef struct {
    MplWgpuDrawRectsFn draw_rects;
    MplWgpuDrawLinesFn draw_lines;
    MplWgpuDrawCirclesFn draw_circles;
    MplWgpuDrawTrianglesFn draw_triangles;
    MplWgpuDrawTextFn draw_text;
    MplWgpuMeasureTextFn measure_text;
    MplWgpuClearFn clear;
    MplWgpuDrawImageFn draw_image;
} MplWgpuVTable;

// ===========================================================================
// Backend API
// ===========================================================================

// Creates a new backend instance using the provided callbacks
MplWgpuBackend* mpl_wgpu_backend_create(const MplWgpuVTable* vtable, void* user_data);

// Destroys the backend instance
void mpl_wgpu_backend_destroy(MplWgpuBackend* backend);

// Renders data (triggers callbacks)
bool mpl_wgpu_backend_render_data(MplWgpuBackend* backend);

// Resizing
void mpl_wgpu_backend_set_size(MplWgpuBackend* backend, unsigned int width, unsigned int height);

// ===========================================================================
// Plotting API (High-level)
// ===========================================================================

// Axis / Figure management
MplFigure* mpl_figure_create(MplWgpuBackend* backend);
void mpl_figure_destroy(MplFigure* fig);

// Old flattened API removed in favor of Axes API

#ifdef __cplusplus
}
#endif
