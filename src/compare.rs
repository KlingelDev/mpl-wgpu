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

/// Generates a diff heatmap as RGBA pixels.
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

#[cfg(test)]
mod tests {
  use super::*;

  /// Helper: build a solid RGBA image of given dimensions and color.
  fn solid(w: u32, h: u32, r: u8, g: u8, b: u8, a: u8) -> Vec<u8> {
    let n = (w * h) as usize;
    let mut buf = Vec::with_capacity(n * 4);
    for _ in 0..n {
      buf.extend_from_slice(&[r, g, b, a]);
    }
    buf
  }

  #[test]
  fn identical_images_zero_error() {
    let img = solid(4, 4, 128, 64, 32, 255);
    let result = compare_images(&img, &img, 4, 4);
    assert_eq!(result.rmse, 0.0);
    assert_eq!(result.max_diff, 0);
    assert_eq!(result.diff_count, 0);
    assert_eq!(result.diff_pct, 0.0);
  }

  #[test]
  fn opposite_images_maximum_error() {
    let black = solid(2, 2, 0, 0, 0, 0);
    let white = solid(2, 2, 255, 255, 255, 255);
    let result = compare_images(&black, &white, 2, 2);
    assert_eq!(result.max_diff, 255);
    // RMSE of all channels being 255 apart = 255.0
    assert!((result.rmse - 255.0).abs() < 1e-9);
    assert_eq!(result.diff_count, 4);
    assert_eq!(result.diff_pct, 100.0);
  }

  #[test]
  fn soft_threshold_boundary_not_counted() {
    // Difference of exactly SOFT_THRESHOLD (5) must NOT count.
    let a = solid(1, 1, 10, 0, 0, 0);
    let b = solid(1, 1, 15, 0, 0, 0);
    let result = compare_images(&a, &b, 1, 1);
    assert_eq!(result.max_diff, 5);
    assert_eq!(result.diff_count, 0);
    assert_eq!(result.diff_pct, 0.0);
  }

  #[test]
  fn soft_threshold_boundary_exceeds() {
    // Difference of SOFT_THRESHOLD + 1 (6) must count.
    let a = solid(1, 1, 10, 0, 0, 0);
    let b = solid(1, 1, 16, 0, 0, 0);
    let result = compare_images(&a, &b, 1, 1);
    assert_eq!(result.max_diff, 6);
    assert_eq!(result.diff_count, 1);
    assert_eq!(result.diff_pct, 100.0);
  }

  #[test]
  fn partial_diff_count_and_pct() {
    // 1x2 image: left pixel identical, right pixel differs.
    let actual = vec![
      128u8, 128, 128, 255, // pixel 0 – identical
      200, 200, 200, 255,   // pixel 1 – differs by 72
    ];
    let expected = vec![
      128u8, 128, 128, 255,
      128, 128, 128, 255,
    ];
    let result = compare_images(&actual, &expected, 2, 1);
    assert_eq!(result.diff_count, 1);
    assert_eq!(result.diff_pct, 50.0);
  }

  #[test]
  fn diff_pixels_matching_pixel_is_green() {
    // Pixels within the soft threshold → green (0, 128, 0, 255).
    let img = solid(1, 1, 10, 10, 10, 255);
    let out = diff_pixels(&img, &img);
    assert_eq!(out.len(), 4);
    assert_eq!(out[0], 0);
    assert_eq!(out[1], 128);
    assert_eq!(out[2], 0);
    assert_eq!(out[3], 255);
  }

  #[test]
  fn diff_pixels_different_pixel_is_red() {
    // Max channel difference = 255 → red channel = min(2550, 255) = 255.
    let a = solid(1, 1, 0, 0, 0, 255);
    let b = solid(1, 1, 255, 0, 0, 255);
    let out = diff_pixels(&a, &b);
    assert_eq!(out.len(), 4);
    assert_eq!(out[0], 255); // red
    assert_eq!(out[1], 0);   // green
    assert_eq!(out[2], 0);   // blue
    assert_eq!(out[3], 255); // alpha always 255
  }

  #[test]
  fn diff_pixels_amplification_capped_at_255() {
    // Difference of 26 → 26 * 10 = 260, capped to 255.
    let a = solid(1, 1, 0, 0, 0, 0);
    let b = solid(1, 1, 26, 0, 0, 0);
    let out = diff_pixels(&a, &b);
    assert_eq!(out[0], 255);
  }

  #[test]
  fn compare_images_output_length_invariant() {
    let w = 10u32;
    let h = 8u32;
    let img = solid(w, h, 0, 0, 0, 255);
    let result = compare_images(&img, &img, w, h);
    // No assertion on pixel-count — just verify it doesn't panic
    // and diff_count cannot exceed total pixels.
    assert!(result.diff_count <= (w * h) as usize);
  }
}
