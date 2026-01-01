// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

//! Rust bindings for mpl-wgpu
//!
//! This crate provides safe Rust bindings to the mpl-wgpu C++ backend,
//! allowing you to use matplotplusplus from Rust with GPU-accelerated
//! rendering via wgpu.
//!
//! # Architecture
//!
//! ```text
//! matplotplusplus (C++) → WgpuBackend (C++) → C FFI → Rust
//! ```
//!
//! # Example
//!
//! ```rust,no_run
//! use mpl_wgpu::Figure;
//!
//! let mut fig = Figure::new();
//! fig.plot(&[1.0, 2.0, 3.0], &[1.0, 4.0, 9.0]);
//! fig.show();
//! ```

#![warn(missing_docs)]
#![warn(rust_2018_idioms)]

pub mod backend;
pub mod ffi;
pub mod primitives;
pub mod text;

pub use backend::{Figure, WgpuBackend};

// Re-export rendering components
pub use primitives::{Instance, PrimitiveRenderer};
pub use text::TextRenderer;

/// Re-export wgpu types for convenience
pub use wgpu;

/// Re-export glam for math types
pub use glam;

/// Library version
pub const VERSION: &str = env!("CARGO_PKG_VERSION");

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_version() {
        assert!(!VERSION.is_empty());
    }
}
