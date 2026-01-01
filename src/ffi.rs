// Copyright (c) 2026 Karl Ruskowski  
// SPDX-License-Identifier: MIT

//! FFI bindings to the C++ mpl-wgpu backend
//!
//! This module provides low-level FFI bindings. Use the safe
//! wrappers in the parent module instead.

#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(dead_code)]

// TODO: Generate with bindgen in build.rs
// For now, manual placeholder FFI declarations

use std::os::raw::{c_char, c_double, c_float, c_uint, c_void};

/// Opaque handle to WgpuBackend
#[repr(C)]
pub struct MplWgpuBackend {
    _private: [u8; 0],
}

/// Opaque handle to WgpuRenderer
#[repr(C)]
pub struct MplWgpuRenderer {
    _private: [u8; 0],
}

/// Opaque handle to matplot++ Figure
#[repr(C)]
pub struct MplFigure {
    _private: [u8; 0],
}

extern "C" {
    // Backend lifecycle
    pub fn mpl_wgpu_backend_create(
        renderer: *mut MplWgpuRenderer,
    ) -> *mut MplWgpuBackend;
    
    pub fn mpl_wgpu_backend_destroy(backend: *mut MplWgpuBackend);

    // Rendering
    pub fn mpl_wgpu_backend_new_frame(
        backend: *mut MplWgpuBackend,
    ) -> bool;
    
    pub fn mpl_wgpu_backend_render_data(
        backend: *mut MplWgpuBackend,
    ) -> bool;

    // Dimensions
    pub fn mpl_wgpu_backend_width(
        backend: *mut MplWgpuBackend,
    ) -> c_uint;
    
    pub fn mpl_wgpu_backend_height(
        backend: *mut MplWgpuBackend,
    ) -> c_uint;
    
    pub fn mpl_wgpu_backend_set_width(
        backend: *mut MplWgpuBackend,
        width: c_uint,
    );
    
    pub fn mpl_wgpu_backend_set_height(
        backend: *mut MplWgpuBackend,
        height: c_uint,
    );
}
