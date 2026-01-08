// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

//! FFI bindings to the C++ mpl-wgpu backend

#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(dead_code)]

use std::os::raw::{c_char, c_float, c_void, c_int, c_double};

// Opaque types
#[repr(C)]
pub struct MplWgpuBackend { _private: [u8; 0] }

#[repr(C)]
pub struct MplFigure { _private: [u8; 0] }

#[repr(C)]
pub struct MplAxes { _private: [u8; 0] }

// Primitive Structures matching C API
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct MplWgpuRect {
    pub x: f32, pub y: f32, pub width: f32, pub height: f32,
    pub r: f32, pub g: f32, pub b: f32, pub a: f32,
    pub stroke_width: f32,
    pub corner_radius: f32,
    pub z: f32,
    pub padding: f32,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct MplWgpuLine {
    pub x1: f32, pub y1: f32, pub z1: f32,
    pub x2: f32, pub y2: f32, pub z2: f32,
    pub r: f32, pub g: f32, pub b: f32, pub a: f32,
    pub width: f32,
    pub dash_len: f32,
    pub gap_len: f32,
    pub dash_offset: f32,
    pub padding: f32,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct MplWgpuCircle {
    pub cx: f32, pub cy: f32, pub cz: f32, pub radius: f32,
    pub r: f32, pub g: f32, pub b: f32, pub a: f32,
    pub type_: f32,
    pub _p1: f32, pub _p2: f32, pub _p3: f32,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct MplWgpuTriangle {
    pub x1: f32, pub y1: f32, pub z1: f32, pub _p1: f32,
    pub x2: f32, pub y2: f32, pub z2: f32, pub _p2: f32,
    pub x3: f32, pub y3: f32, pub z3: f32, pub _p3: f32,
    pub nx1: f32, pub ny1: f32, pub nz1: f32, pub _p4: f32,
    pub nx2: f32, pub ny2: f32, pub nz2: f32, pub _p5: f32,
    pub nx3: f32, pub ny3: f32, pub nz3: f32, pub _p6: f32,
    pub r: f32, pub g: f32, pub b: f32, pub a: f32,
}

// Callbacks
pub type MplWgpuDrawRectsFn = extern "C" fn(user_data: *mut c_void, rects: *const MplWgpuRect, count: usize);
pub type MplWgpuDrawLinesFn = extern "C" fn(user_data: *mut c_void, lines: *const MplWgpuLine, count: usize);
pub type MplWgpuDrawCirclesFn = extern "C" fn(user_data: *mut c_void, circles: *const MplWgpuCircle, count: usize);
pub type MplWgpuDrawTrianglesFn = extern "C" fn(user_data: *mut c_void, triangles: *const MplWgpuTriangle, count: usize);
pub type MplWgpuDrawTextFn = extern "C" fn(user_data: *mut c_void, text: *const c_char, x: f32, y: f32, font_size: f32, r: f32, g: f32, b: f32, a: f32, rotation: f32);
pub type MplWgpuMeasureTextFn = extern "C" fn(user_data: *mut c_void, text: *const c_char, font_size: f32) -> f32;
pub type MplWgpuClearFn = extern "C" fn(user_data: *mut c_void, r: f32, g: f32, b: f32, a: f32);
pub type MplWgpuDrawImageFn = extern "C" fn(user_data: *mut c_void, data: *const c_float, img_width: usize, img_height: usize, x: f32, y: f32, width: f32, height: f32);

#[repr(C)]
pub struct MplWgpuVTable {
    pub draw_rects: Option<MplWgpuDrawRectsFn>,
    pub draw_lines: Option<MplWgpuDrawLinesFn>,
    pub draw_circles: Option<MplWgpuDrawCirclesFn>,
    pub draw_triangles: Option<MplWgpuDrawTrianglesFn>,
    pub draw_text: Option<MplWgpuDrawTextFn>,
    pub measure_text: Option<MplWgpuMeasureTextFn>,
    pub clear: Option<MplWgpuClearFn>,
    pub draw_image: Option<MplWgpuDrawImageFn>,
}

extern "C" {
    pub fn mpl_wgpu_backend_create(vtable: *const MplWgpuVTable, user_data: *mut c_void) -> *mut MplWgpuBackend;
    pub fn mpl_wgpu_backend_destroy(backend: *mut MplWgpuBackend);
    pub fn mpl_wgpu_backend_render_data(backend: *mut MplWgpuBackend) -> bool;
    pub fn mpl_wgpu_backend_set_size(backend: *mut MplWgpuBackend, width: c_uint, height: c_uint);

    pub fn mpl_figure_create(backend: *mut MplWgpuBackend) -> *mut MplFigure;
    pub fn mpl_figure_destroy(fig: *mut MplFigure);
    pub fn mpl_figure_current_axes(fig: *mut MplFigure) -> *mut MplAxes;
    pub fn mpl_figure_clear(fig: *mut MplFigure);

    pub fn mpl_axes_destroy(ax: *mut MplAxes);
    pub fn mpl_axes_plot(ax: *mut MplAxes, x: *const c_double, y: *const c_double, count: usize, style: *const c_char);
    pub fn mpl_axes_scatter(ax: *mut MplAxes, x: *const c_double, y: *const c_double, count: usize, style: *const c_char);
    pub fn mpl_axes_bar(ax: *mut MplAxes, values: *const c_double, count: usize);
    pub fn mpl_axes_hist(ax: *mut MplAxes, values: *const c_double, count: usize, bins: usize);
    pub fn mpl_axes_surface(ax: *mut MplAxes, x: *const c_double, y: *const c_double, z: *const c_double, rows: usize, cols: usize, wireframe: bool);
    pub fn mpl_axes_pie(ax: *mut MplAxes, values: *const c_double, count: usize);
    pub fn mpl_axes_boxplot(ax: *mut MplAxes, values: *const c_double, count: usize);
    pub fn mpl_axes_heatmap(ax: *mut MplAxes, z: *const c_double, rows: usize, cols: usize);
    
    pub fn mpl_axes_set_title(ax: *mut MplAxes, title: *const c_char);
    pub fn mpl_axes_set_xlabel(ax: *mut MplAxes, label: *const c_char);
    pub fn mpl_axes_set_ylabel(ax: *mut MplAxes, label: *const c_char);
    pub fn mpl_axes_grid(ax: *mut MplAxes, on: bool);
    pub fn mpl_axes_set_xlim(ax: *mut MplAxes, min: c_double, max: c_double);
    pub fn mpl_axes_set_ylim(ax: *mut MplAxes, min: c_double, max: c_double);
}

// Re-export c_uint for convenience
use std::os::raw::c_uint;
