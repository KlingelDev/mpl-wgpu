//! # mpl-wgpu
//!
//! A matplotlib-style plotting library for Rust with hardware-accelerated 
//! rendering using [wgpu](https://wgpu.rs/).
//!
//! ## Features
//!
//! - GPU-accelerated rendering for all plot types
//! - Line plots, scatter plots, histograms, bar charts, and 3D surfaces
//! - Customizable colors, line styles, markers, and labels
//! - Automatic or manual axis scaling
//! - Real-time plot updates
//!
//! ## Quick Start
//!
//! ```rust,no_run
//! use mpl_wgpu::{PlotBackend, PrimitiveRenderer, TextRenderer};
//! use glam::Vec4;
//!
//! // Create plot backend
//! let mut plot = PlotBackend::new(800, 600);
//!
//! // Configure axes
//! plot.axis_mut().title = "My Plot".into();
//! plot.axis_mut().x_label = "X Axis".into();
//! plot.axis_mut().y_label = "Y Axis".into();
//!
//! // Add data
//! let x: Vec<f64> = (0..100).map(|i| i as f64 * 0.1).collect();
//! let y: Vec<f64> = x.iter().map(|&x| x.sin()).collect();
//! let color = Vec4::new(0.0, 0.5, 1.0, 1.0);
//!
//! plot.plot(x, y, color);
//! plot.auto_scale();
//! ```
//!
//! ## Architecture
//!
//! The library is built on three core components:
//!
//! - [`PlotBackend`]: High-level matplotlib-style plotting API
//! - [`PrimitiveRenderer`]: Low-level GPU primitive rendering
//! - [`TextRenderer`]: GPU-accelerated text rendering

/// Plotting backend with matplotlib-style API.
pub mod plotting;

/// Low-level GPU primitive rendering (lines, circles, rectangles).
pub mod primitives;

/// GPU-accelerated text rendering.
pub mod text;

pub use plotting::{PlotBackend, AxisConfig, Series, LineStyle, MarkerStyle, linspace, randn, PI};
pub use primitives::{PrimitiveRenderer, Instance};
pub use text::TextRenderer;

pub use glam::{Vec2, Vec3, Vec4}; 
