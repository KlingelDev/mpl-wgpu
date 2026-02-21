// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

//! Shared utilities for visual regression tests.

use mpl_wgpu::capture::PlotCapture;
use mpl_wgpu::compare;
use std::path::PathBuf;

/// Returns the path to a golden reference PNG.
pub fn golden_path(name: &str) -> PathBuf {
  PathBuf::from(env!("CARGO_MANIFEST_DIR"))
    .join("tests")
    .join("golden")
    .join(format!("{}.png", name))
}

/// Returns the path for test output artifacts.
pub fn output_path(name: &str) -> PathBuf {
  PathBuf::from(env!("CARGO_MANIFEST_DIR"))
    .join("tests")
    .join("output")
    .join(format!("{}.png", name))
}

/// Default comparison thresholds.
const DEFAULT_MAX_RMSE: f64 = 2.0;
const DEFAULT_MAX_DIFF_PCT: f64 = 2.0;

/// Orchestrates a visual regression test.
///
/// Creates a [`PlotCapture`] at 800x600, calls `setup_fn` to
/// configure the plot, captures the result, and either blesses
/// (when `BLESS=1`) or compares against the golden reference.
/// On failure, saves actual + diff images to `tests/output/`.
pub fn run_visual_test<F>(name: &str, setup_fn: F)
where
  F: FnOnce(&PlotCapture),
{
  let cap = PlotCapture::new(800, 600);
  setup_fn(&cap);
  run_visual_test_with_capture(name, cap);
}

/// Like [`run_visual_test`] but takes an already-configured
/// [`PlotCapture`], allowing the caller full control.
pub fn run_visual_test_with_capture(
  name: &str,
  mut cap: PlotCapture,
) {
  let actual = cap.render_and_capture();
  let w = cap.width();
  let h = cap.height();

  let golden = golden_path(name);
  let bless = std::env::var("BLESS").is_ok();

  if bless {
    image::save_buffer(
      &golden,
      &actual,
      w,
      h,
      image::ColorType::Rgba8,
    )
    .expect("Failed to bless golden image");
    eprintln!("Blessed golden: {}", golden.display());
    return;
  }

  if !golden.exists() {
    panic!(
      "Golden file missing: {}. Run with BLESS=1 to generate.",
      golden.display()
    );
  }

  let expected_img = image::open(&golden)
    .expect("Failed to open golden image")
    .to_rgba8();

  assert_eq!(
    expected_img.width(),
    w,
    "Golden width mismatch"
  );
  assert_eq!(
    expected_img.height(),
    h,
    "Golden height mismatch"
  );

  let expected = expected_img.as_raw();
  let result =
    compare::compare_images(&actual, expected, w, h);

  if result.rmse > DEFAULT_MAX_RMSE
    || result.diff_pct > DEFAULT_MAX_DIFF_PCT
  {
    // Save actual and diff for inspection.
    let actual_path =
      output_path(&format!("{}_actual", name));
    image::save_buffer(
      &actual_path,
      &actual,
      w,
      h,
      image::ColorType::Rgba8,
    )
    .ok();

    let diff_path =
      output_path(&format!("{}_diff", name));
    let diff_buf =
      compare::diff_pixels(&actual, expected);
    image::save_buffer(
      &diff_path,
      &diff_buf,
      w,
      h,
      image::ColorType::Rgba8,
    )
    .expect("Failed to save diff image");

    panic!(
      "Visual regression failed for '{}': \
       RMSE={:.2} (max {:.2}), \
       diff={:.2}% (max {:.2}%)\n\
       Actual: {}\n\
       Diff:   {}",
      name,
      result.rmse,
      DEFAULT_MAX_RMSE,
      result.diff_pct,
      DEFAULT_MAX_DIFF_PCT,
      actual_path.display(),
      diff_path.display(),
    );
  }
}
