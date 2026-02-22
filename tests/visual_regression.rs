// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

//! Visual regression tests for mpl-wgpu.
//!
//! Each test renders a plot through the full matplot++ -> wgpu
//! pipeline and compares the output against a golden reference PNG.
//!
//! # Usage
//!
//! Generate golden files (inspect manually before committing):
//! ```sh
//! BLESS=1 cargo test --test visual_regression
//! ```
//!
//! Run regression checks:
//! ```sh
//! cargo test --test visual_regression
//! ```
//!
//! On failure, actual and diff images are saved to `tests/output/`.

mod common;

use mpl_wgpu::test_cases;
use serial_test::serial;

/// Looks up a test case by name and runs it through the visual
/// regression harness.
fn run(name: &str) {
  let cases = test_cases::all();
  let tc = cases
    .iter()
    .find(|c| c.name == name)
    .unwrap_or_else(|| {
      panic!("Unknown test case: {}", name)
    });
  common::run_visual_test(tc.name, tc.setup);
}

#[test]
#[serial]
fn test_line_plot() {
  run("line_plot");
}

#[test]
#[serial]
fn test_scatter_plot() {
  run("scatter_plot");
}

#[test]
#[serial]
fn test_bar_chart() {
  run("bar_chart");
}

#[test]
#[serial]
fn test_multi_line() {
  run("multi_line");
}

#[test]
#[serial]
fn test_histogram() {
  run("histogram");
}

#[test]
#[serial]
fn test_grid_and_labels() {
  run("grid_and_labels");
}

#[test]
#[serial]
fn test_heatmap() {
  run("heatmap");
}

#[test]
#[serial]
fn test_surface_3d() {
  run("surface_3d");
}

#[test]
#[serial]
fn test_pie_chart() {
  run("pie_chart");
}

#[test]
#[serial]
fn test_box_chart() {
  run("box_chart");
}
