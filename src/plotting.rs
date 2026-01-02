// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

//! High-level plotting API for mpl-wgpu

use crate::ffi;
use crate::primitives::PrimitiveRenderer;
use crate::text::TextRenderer;
use std::ffi::{CString, CStr};
use std::os::raw::{c_void, c_char};
use glam::{Mat4, Vec2, Vec3, Vec4};

// Re-exports
pub use std::f64::consts::PI;

pub fn linspace(start: f64, end: f64, n: usize) -> Vec<f64> {
    if n == 0 { return Vec::new(); }
    if n == 1 { return vec![start]; }
    let step = (end - start) / (n - 1) as f64;
    (0..n).map(|i| start + i as f64 * step).collect()
}

pub fn randn(n: usize) -> Vec<f64> {
    let mut v = Vec::with_capacity(n);
    let mut seed: u64 = 123456789;
    for _ in 0..n {
        seed = seed.wrapping_mul(6364136223846793005).wrapping_add(1);
        let valid = (seed >> 32) as f64 / 4294967296.0;
        v.push(valid * 2.0 - 1.0); 
    }
    v
}

// ----------------------------------------------------------------------------
// Axes
// ----------------------------------------------------------------------------

pub struct Axes {
    ptr: *mut ffi::MplAxes,
}

impl Axes {
    pub fn plot(&self, x: &[f64], y: &[f64], style: &str) {
        let c_style = CString::new(style).unwrap_or_default();
        unsafe {
            ffi::mpl_axes_plot(self.ptr, x.as_ptr(), y.as_ptr(), x.len().min(y.len()), c_style.as_ptr());
        }
    }

    pub fn scatter(&self, x: &[f64], y: &[f64], style: &str) {
        let c_style = CString::new(style).unwrap_or_default();
        unsafe {
            ffi::mpl_axes_scatter(self.ptr, x.as_ptr(), y.as_ptr(), x.len().min(y.len()), c_style.as_ptr());
        }
    }
    
    pub fn bar(&self, values: &[f64]) {
        unsafe { ffi::mpl_axes_bar(self.ptr, values.as_ptr(), values.len()); }
    }
    
    pub fn hist(&self, values: &[f64], bins: usize) {
        unsafe { ffi::mpl_axes_hist(self.ptr, values.as_ptr(), values.len(), bins); }
    }
    
    /// Surface plot. x, y, z must be flattened pointers to meshgrid data of size rows * cols.
    pub fn surf(&self, x: &[f64], y: &[f64], z: &[f64], rows: usize, cols: usize, wireframe: bool) {
        // Validation?
        unsafe {
            ffi::mpl_axes_surface(self.ptr, x.as_ptr(), y.as_ptr(), z.as_ptr(), rows, cols, wireframe);
        }
    }

    pub fn set_title(&self, text: &str) {
        let c_text = CString::new(text).unwrap_or_default();
        unsafe { ffi::mpl_axes_set_title(self.ptr, c_text.as_ptr()); }
    }

    pub fn set_xlabel(&self, text: &str) {
        let c_text = CString::new(text).unwrap_or_default();
        unsafe { ffi::mpl_axes_set_xlabel(self.ptr, c_text.as_ptr()); }
    }

    pub fn set_ylabel(&self, text: &str) {
        let c_text = CString::new(text).unwrap_or_default();
        unsafe { ffi::mpl_axes_set_ylabel(self.ptr, c_text.as_ptr()); }
    }
    
    pub fn grid(&self, on: bool) {
        unsafe { ffi::mpl_axes_grid(self.ptr, on); }
    }
    
    pub fn set_xlim(&self, min: f64, max: f64) {
        unsafe { ffi::mpl_axes_set_xlim(self.ptr, min, max); }
    }
    
    pub fn set_ylim(&self, min: f64, max: f64) {
        unsafe { ffi::mpl_axes_set_ylim(self.ptr, min, max); }
    }
}

impl Drop for Axes {
    fn drop(&mut self) {
        unsafe { ffi::mpl_axes_destroy(self.ptr); }
    }
}

// ----------------------------------------------------------------------------
// Figure
// ----------------------------------------------------------------------------

pub struct Figure {
    ptr: *mut ffi::MplFigure,
}

impl Figure {
    pub fn current_axes(&self) -> Axes {
        let ptr = unsafe { ffi::mpl_figure_current_axes(self.ptr) };
        Axes { ptr }
    }
    
    pub fn clear(&self) {
        unsafe { ffi::mpl_figure_clear(self.ptr); }
    }
}

// ----------------------------------------------------------------------------
// PlotBackend
// ----------------------------------------------------------------------------

struct BackendContext {
    prim: *mut PrimitiveRenderer,
    text: *mut TextRenderer,
}

extern "C" fn draw_rects_cb(user_data: *mut c_void, rects: *const ffi::MplWgpuRect, count: usize) {
    let ctx = unsafe { &mut *(user_data as *mut BackendContext) };
    if ctx.prim.is_null() { return; }
    let prim = unsafe { &mut *ctx.prim };
    let rects_slice = unsafe { std::slice::from_raw_parts(rects, count) };
    for r in rects_slice {
        prim.draw_rect(
            Vec2::new(r.x, r.y), 
            Vec2::new(r.width, r.height), 
            Vec4::new(r.r, r.g, r.b, r.a),
            r.corner_radius, 
            r.stroke_width
        );
    }
}

extern "C" fn draw_lines_cb(user_data: *mut c_void, lines: *const ffi::MplWgpuLine, count: usize) {
    let ctx = unsafe { &mut *(user_data as *mut BackendContext) };
    if ctx.prim.is_null() { return; }
    let prim = unsafe { &mut *ctx.prim };
    let lines_slice = unsafe { std::slice::from_raw_parts(lines, count) };
    for l in lines_slice {
        prim.draw_line(
            Vec3::new(l.x1, l.y1, l.z1),
            Vec3::new(l.x2, l.y2, l.z2),
            l.width,
            Vec4::new(l.r, l.g, l.b, l.a),
            l.dash_len,
            l.gap_len,
            l.dash_offset
        );
    }
}

extern "C" fn draw_circles_cb(user_data: *mut c_void, circles: *const ffi::MplWgpuCircle, count: usize) {
    let ctx = unsafe { &mut *(user_data as *mut BackendContext) };
    if ctx.prim.is_null() { return; }
    let prim = unsafe { &mut *ctx.prim };
    let slice = unsafe { std::slice::from_raw_parts(circles, count) };
    for c in slice {
        prim.draw_circle(
            Vec3::new(c.cx, c.cy, c.cz),
            c.radius,
            Vec4::new(c.r, c.g, c.b, c.a),
            0.0, c.type_ as u32
        );
    }
}

extern "C" fn draw_triangles_cb(user_data: *mut c_void, tris: *const ffi::MplWgpuTriangle, count: usize) {
    let ctx = unsafe { &mut *(user_data as *mut BackendContext) };
    if ctx.prim.is_null() { return; }
    let prim = unsafe { &mut *ctx.prim };
    let slice = unsafe { std::slice::from_raw_parts(tris, count) };
    for t in slice {
        prim.draw_triangle(
            Vec3::new(t.x1, t.y1, t.z1),
            Vec3::new(t.x2, t.y2, t.z2),
            Vec3::new(t.x3, t.y3, t.z3),
            Vec4::new(t.r, t.g, t.b, t.a)
        );
    }
}

extern "C" fn draw_text_cb(user_data: *mut c_void, text: *const c_char, x: f32, y: f32, size: f32, r: f32, g: f32, b: f32, a: f32, _rot: f32) {
    let ctx = unsafe { &mut *(user_data as *mut BackendContext) };
    if ctx.text.is_null() { return; }
    let text_renderer = unsafe { &mut *ctx.text };
    if text.is_null() { return; }
    let c_str = unsafe { CStr::from_ptr(text) };
    if let Ok(s) = c_str.to_str() {
       text_renderer.draw_text(s, Vec2::new(x, y), size, Vec4::new(r, g, b, a));
    }
}

extern "C" fn measure_text_cb(user_data: *mut c_void, text: *const c_char, size: f32) -> f32 {
    if text.is_null() { return 0.0; }
    let c_str = unsafe { CStr::from_ptr(text) };
    let ctx = unsafe { &mut *(user_data as *mut BackendContext) };
    if !ctx.text.is_null() {
        let text_renderer = unsafe { &mut *ctx.text };
        if let Ok(s) = c_str.to_str() {
            return text_renderer.measure_text(s, size).x;
        }
    }
    // Fallback
    c_str.to_bytes().len() as f32 * size * 0.6
}

extern "C" fn clear_cb(_user_data: *mut c_void, _r: f32, _g: f32, _b: f32, _a: f32) { }

extern "C" fn draw_image_cb(_user_data: *mut c_void, _data: *const f32, _w: usize, _h: usize, _x: f32, _y: f32, _dw: f32, _dh: f32) { }

static VTABLE: ffi::MplWgpuVTable = ffi::MplWgpuVTable {
    draw_rects: Some(draw_rects_cb),
    draw_lines: Some(draw_lines_cb),
    draw_circles: Some(draw_circles_cb),
    draw_triangles: Some(draw_triangles_cb),
    draw_text: Some(draw_text_cb),
    measure_text: Some(measure_text_cb),
    clear: Some(clear_cb),
    draw_image: Some(draw_image_cb),
};

pub struct PlotBackend {
    width: u32,
    height: u32,
    backend_ptr: *mut ffi::MplWgpuBackend,
    figure_ptr: *mut ffi::MplFigure,
    ctx_ptr: *mut BackendContext,
}

impl PlotBackend {
    pub fn new(width: u32, height: u32) -> Self {
        let ctx = Box::new(BackendContext {
            prim: std::ptr::null_mut(),
            text: std::ptr::null_mut(),
        });
        
        let ctx_ptr = Box::into_raw(ctx);
        
        let backend_ptr = unsafe { 
            ffi::mpl_wgpu_backend_create(&VTABLE, ctx_ptr as *mut c_void) 
        };
        
        unsafe {
            ffi::mpl_wgpu_backend_set_size(backend_ptr, width, height);
        }
        
        let figure_ptr = unsafe {
            ffi::mpl_figure_create(backend_ptr)
        };
        
        Self {
            width,
            height,
            backend_ptr,
            figure_ptr,
            ctx_ptr,
        }
    }
    
    // Provide access to the figure
    // Note: In C++, backend doesn't usually own figure, but here we kind of do to keep it alive.
    // Or we view PlotBackend as the "Canvas + Window" which holds the figure logic.
    pub fn figure(&self) -> Figure {
        Figure { ptr: self.figure_ptr }
    }
    
    pub fn resize(&mut self, width: u32, height: u32) {
        self.width = width;
        self.height = height;
        unsafe { ffi::mpl_wgpu_backend_set_size(self.backend_ptr, width, height); }
    }

    pub fn set_scale_factor(&mut self, _scale: f32) {}

    pub fn render(&mut self, prim: &mut PrimitiveRenderer, text: &mut TextRenderer, _target: Option<Mat4>) {
        unsafe {
            (*self.ctx_ptr).prim = prim as *mut _;
            (*self.ctx_ptr).text = text as *mut _;
            ffi::mpl_wgpu_backend_render_data(self.backend_ptr);
            (*self.ctx_ptr).prim = std::ptr::null_mut();
            (*self.ctx_ptr).text = std::ptr::null_mut();
        }
    }
}

impl Drop for PlotBackend {
    fn drop(&mut self) {
        unsafe {
            ffi::mpl_figure_destroy(self.figure_ptr);
            ffi::mpl_wgpu_backend_destroy(self.backend_ptr);
            let _ = Box::from_raw(self.ctx_ptr);
        }
    }
}
