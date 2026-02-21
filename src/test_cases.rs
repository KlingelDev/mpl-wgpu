// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

//! Canonical registry of visual test cases.
//!
//! Each test case pairs a name (used as the golden file stem) with a
//! setup function that configures a [`PlotCapture`].  Both the
//! automated regression tests and the interactive review GUI import
//! from here so the definitions stay in sync.

use crate::capture::PlotCapture;
use crate::plotting::linspace;

/// A named visual test case.
pub struct TestCase {
  /// Display name and golden-file stem.
  pub name: &'static str,
  /// Configures the plot on a [`PlotCapture`].
  pub setup: fn(&PlotCapture),
}

/// Returns the full list of visual test cases.
pub fn all() -> Vec<TestCase> {
  vec![
    TestCase { name: "line_plot", setup: setup_line_plot },
    TestCase { name: "scatter_plot", setup: setup_scatter_plot },
    TestCase { name: "bar_chart", setup: setup_bar_chart },
    TestCase { name: "multi_line", setup: setup_multi_line },
    TestCase { name: "histogram", setup: setup_histogram },
    TestCase {
      name: "grid_and_labels",
      setup: setup_grid_and_labels,
    },
    TestCase { name: "heatmap", setup: setup_heatmap },
    TestCase { name: "surface_3d", setup: setup_surface_3d },
  ]
}

/// Sine wave with title and axis labels.
fn setup_line_plot(cap: &PlotCapture) {
  let fig = cap.figure();
  let ax = fig.current_axes();
  let x = linspace(0.0, 6.283, 100);
  let y: Vec<f64> = x.iter().map(|v| v.sin()).collect();
  ax.plot(&x, &y, "-");
  ax.set_title("Sine Wave");
  ax.set_xlabel("x");
  ax.set_ylabel("sin(x)");
}

/// Scatter plot with deterministic pseudo-random points.
fn setup_scatter_plot(cap: &PlotCapture) {
  let fig = cap.figure();
  let ax = fig.current_axes();
  let x = crate::plotting::randn(80);
  let y = {
    let mut v = crate::plotting::randn(80);
    v.iter_mut().enumerate().for_each(|(i, val)| {
      *val = (*val + (i as f64 * 0.01)).sin();
    });
    v
  };
  ax.scatter(&x, &y, "o");
  ax.set_title("Scatter Plot");
}

/// Bar chart with 5 categories.
fn setup_bar_chart(cap: &PlotCapture) {
  let fig = cap.figure();
  let ax = fig.current_axes();
  ax.bar(&[3.0, 7.0, 5.0, 9.0, 2.0]);
  ax.set_title("Bar Chart");
}

/// Three overlaid curves with different styles.
fn setup_multi_line(cap: &PlotCapture) {
  let fig = cap.figure();
  let ax = fig.current_axes();
  let x = linspace(0.0, 6.283, 100);
  let y1: Vec<f64> = x.iter().map(|v| v.sin()).collect();
  let y2: Vec<f64> = x.iter().map(|v| v.cos()).collect();
  let y3: Vec<f64> =
    x.iter().map(|v| (v * 2.0).sin() * 0.5).collect();
  ax.plot(&x, &y1, "-");
  ax.plot(&x, &y2, "-");
  ax.plot(&x, &y3, "-");
  ax.set_title("Multiple Lines");
}

/// Histogram with deterministic pseudo-random data.
fn setup_histogram(cap: &PlotCapture) {
  let fig = cap.figure();
  let ax = fig.current_axes();
  let data = crate::plotting::randn(200);
  ax.hist(&data, 20);
  ax.set_title("Histogram");
}

/// Grid lines and axis labels.
fn setup_grid_and_labels(cap: &PlotCapture) {
  let fig = cap.figure();
  let ax = fig.current_axes();
  let x = linspace(-5.0, 5.0, 50);
  let y: Vec<f64> = x.iter().map(|v| v * v).collect();
  ax.plot(&x, &y, "-");
  ax.grid(true);
  ax.set_title("Parabola with Grid");
  ax.set_xlabel("x");
  ax.set_ylabel("x^2");
}

/// 10x10 heatmap.
fn setup_heatmap(cap: &PlotCapture) {
  let fig = cap.figure();
  let ax = fig.current_axes();
  let rows = 10usize;
  let cols = 10usize;
  let mut z = Vec::with_capacity(rows * cols);
  for r in 0..rows {
    for c in 0..cols {
      z.push(
        ((r as f64 - 4.5).powi(2) + (c as f64 - 4.5).powi(2))
          .sqrt(),
      );
    }
  }
  ax.heatmap(&z, rows, cols);
  ax.set_title("Heatmap");
}

/// 3D surface plot (sinc-like function).
fn setup_surface_3d(cap: &PlotCapture) {
  let fig = cap.figure();
  let ax = fig.current_axes();
  let n = 20usize;
  let vals = linspace(-3.0, 3.0, n);
  let mut x = Vec::with_capacity(n * n);
  let mut y = Vec::with_capacity(n * n);
  let mut z = Vec::with_capacity(n * n);
  for r in 0..n {
    for c in 0..n {
      let xv = vals[c];
      let yv = vals[r];
      x.push(xv);
      y.push(yv);
      let rv = (xv * xv + yv * yv).sqrt().max(0.01);
      z.push(rv.sin() / rv);
    }
  }
  ax.surf(&x, &y, &z, n, n, false);
  ax.set_title("3D Surface");
}
