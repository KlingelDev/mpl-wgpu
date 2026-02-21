// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

//! Pixel-level image comparison utilities.
//!
//! Used by both the automated visual regression tests and the
//! interactive review GUI.

/// Per-channel soft threshold for "different enough to count".
const SOFT_THRESHOLD: u8 = 5;

/// Result of comparing two RGBA images pixel-by-pixel.
pub struct CompareResult {
  /// Root mean square error across all channels (0–255 scale).
  pub rmse: f64,
  /// Maximum per-pixel channel difference.
  pub max_diff: u8,
  /// Fraction of pixels exceeding the soft threshold (percent).
  pub diff_pct: f64,
  /// Number of pixels exceeding the soft threshold.
  pub diff_count: usize,
}

/// Compares two RGBA pixel buffers and returns statistics.
///
/// Both buffers must be `width * height * 4` bytes.
pub fn compare_images(
  actual: &[u8],
  expected: &[u8],
  width: u32,
  height: u32,
) -> CompareResult {
  let pixel_count = (width * height) as usize;
  assert_eq!(actual.len(), pixel_count * 4);
  assert_eq!(expected.len(), pixel_count * 4);

  let mut sum_sq: f64 = 0.0;
  let mut max_diff: u8 = 0;
  let mut diff_count: usize = 0;

  for i in 0..pixel_count {
    let off = i * 4;
    let mut pixel_exceeds = false;
    for c in 0..4 {
      let a = actual[off + c] as i32;
      let e = expected[off + c] as i32;
      let d = (a - e).unsigned_abs() as u8;
      sum_sq += (d as f64) * (d as f64);
      if d > max_diff {
        max_diff = d;
      }
      if d > SOFT_THRESHOLD {
        pixel_exceeds = true;
      }
    }
    if pixel_exceeds {
      diff_count += 1;
    }
  }

  let channel_count = (pixel_count * 4) as f64;
  let rmse = (sum_sq / channel_count).sqrt();
  let diff_pct =
    (diff_count as f64 / pixel_count as f64) * 100.0;

  CompareResult {
    rmse,
    max_diff,
    diff_pct,
    diff_count,
  }
}

/// Generates a diff heat-map as RGBA pixels.
///
/// Green = match (within soft threshold), red = differs (amplified
/// 10x).  Returns a buffer of `actual.len()` bytes.
pub fn diff_pixels(actual: &[u8], expected: &[u8]) -> Vec<u8> {
  let pixel_count = actual.len() / 4;
  let mut out = vec![0u8; pixel_count * 4];
  for i in 0..pixel_count {
    let off = i * 4;
    let dr =
      (actual[off] as i32 - expected[off] as i32).abs();
    let dg = (actual[off + 1] as i32
      - expected[off + 1] as i32)
      .abs();
    let db = (actual[off + 2] as i32
      - expected[off + 2] as i32)
      .abs();
    let max_d = dr.max(dg).max(db);
    if max_d <= SOFT_THRESHOLD as i32 {
      out[off] = 0;
      out[off + 1] = 128;
      out[off + 2] = 0;
    } else {
      let a = (max_d * 10).min(255) as u8;
      out[off] = a;
      out[off + 1] = 0;
      out[off + 2] = 0;
    }
    out[off + 3] = 255;
  }
  out
}
