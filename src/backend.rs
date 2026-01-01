// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

//! Safe Rust wrappers around the C++ backend

use crate::ffi;
use anyhow::Result;

/// A matplotplusplus figure with wgpu backend
pub struct Figure {
    // TODO: Implement when C FFI is ready
    _phantom: std::marker::PhantomData<()>,
}

impl Figure {
    /// Creates a new figure
    pub fn new() -> Self {
        Self {
            _phantom: std::marker::PhantomData,
        }
    }

    /// Plots a 2D line
    pub fn plot(&mut self, x: &[f64], y: &[f64]) -> Result<()> {
        // TODO: Call C++ matplot++ via FFI
        Ok(())
    }

    /// Shows the figure
    pub fn show(&self) -> Result<()> {
        // TODO: Call backend render
        Ok(())
    }
}

impl Default for Figure {
    fn default() -> Self {
        Self::new()
    }
}

/// wgpu backend handle
pub struct WgpuBackend {
    // TODO: Implement when C FFI is ready
    _phantom: std::marker::PhantomData<()>,
}

impl WgpuBackend {
    /// Creates a new wgpu backend
    pub fn new() -> Self {
        Self {
            _phantom: std::marker::PhantomData,
        }
    }
}

impl Default for WgpuBackend {
    fn default() -> Self {
        Self::new()
    }
}
