// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

//! Interactive visual review tool for mpl-wgpu plots.
//!
//! Pre-renders all test cases, compares against golden references,
//! and presents an SDL2 window with in-window status bar showing
//! pass/fail/new status and RMSE values.
//!
//! # Usage
//!
//! ```sh
//! cargo run --example test_display --features test-display
//! ```
//!
//! # Key bindings
//!
//! - Left/Right: navigate test cases
//! - 1–8: jump to test case by number
//! - Tab: cycle display mode (Live/Golden/Ref/RefDiff/Diff)
//! - B: bless current test (save render as golden)
//! - A: bless ALL tests
//! - G: generate gnuplot references for all tests
//! - R: re-render current test
//! - S: save current render to tests/output/
//! - Q/Esc: quit

use mpl_wgpu::capture::PlotCapture;
use mpl_wgpu::compare;
use mpl_wgpu::plotting::GnuplotFigure;
use mpl_wgpu::test_cases;
use raw_window_handle::{HasDisplayHandle, HasWindowHandle};
use sdl2::event::Event;
use sdl2::keyboard::Keycode;
use std::path::PathBuf;

const WIDTH: u32 = 800;
const HEIGHT: u32 = 600;
const STATUS_H: u32 = 60;
const WIN_H: u32 = HEIGHT + STATUS_H;

// -----------------------------------------------------------------
// Embedded bitmap font (6x10, printable ASCII 0x20..=0x7E)
// -----------------------------------------------------------------

const FONT_W: usize = 6;
const FONT_H: usize = 10;

/// 6x10 bitmap font covering ASCII 0x20–0x7E (95 glyphs).
/// Each glyph is 10 rows; each row is a u8 bitmask of 6 columns
/// (MSB = leftmost pixel).
#[rustfmt::skip]
const FONT: [[u8; FONT_H]; 95] = [
  // 0x20 ' '
  [0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00],
  // 0x21 '!'
  [0x00,0x20,0x20,0x20,0x20,0x20,0x00,0x20,0x00,0x00],
  // 0x22 '"'
  [0x00,0x50,0x50,0x50,0x00,0x00,0x00,0x00,0x00,0x00],
  // 0x23 '#'
  [0x00,0x50,0xF8,0x50,0x50,0xF8,0x50,0x00,0x00,0x00],
  // 0x24 '$'
  [0x00,0x20,0x70,0xA0,0x70,0x28,0x70,0x20,0x00,0x00],
  // 0x25 '%'
  [0x00,0x48,0xA8,0x50,0x20,0x50,0xA8,0x90,0x00,0x00],
  // 0x26 '&'
  [0x00,0x20,0x50,0x50,0x20,0x54,0x48,0x34,0x00,0x00],
  // 0x27 '''
  [0x00,0x20,0x20,0x20,0x00,0x00,0x00,0x00,0x00,0x00],
  // 0x28 '('
  [0x00,0x10,0x20,0x20,0x20,0x20,0x20,0x10,0x00,0x00],
  // 0x29 ')'
  [0x00,0x20,0x10,0x10,0x10,0x10,0x10,0x20,0x00,0x00],
  // 0x2A '*'
  [0x00,0x00,0x50,0x20,0x70,0x20,0x50,0x00,0x00,0x00],
  // 0x2B '+'
  [0x00,0x00,0x20,0x20,0xF8,0x20,0x20,0x00,0x00,0x00],
  // 0x2C ','
  [0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x40,0x00],
  // 0x2D '-'
  [0x00,0x00,0x00,0x00,0xF8,0x00,0x00,0x00,0x00,0x00],
  // 0x2E '.'
  [0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00],
  // 0x2F '/'
  [0x00,0x08,0x08,0x10,0x20,0x40,0x80,0x80,0x00,0x00],
  // 0x30 '0'
  [0x00,0x70,0x88,0x98,0xA8,0xC8,0x88,0x70,0x00,0x00],
  // 0x31 '1'
  [0x00,0x20,0x60,0x20,0x20,0x20,0x20,0x70,0x00,0x00],
  // 0x32 '2'
  [0x00,0x70,0x88,0x08,0x10,0x20,0x40,0xF8,0x00,0x00],
  // 0x33 '3'
  [0x00,0x70,0x88,0x08,0x30,0x08,0x88,0x70,0x00,0x00],
  // 0x34 '4'
  [0x00,0x10,0x30,0x50,0x90,0xF8,0x10,0x10,0x00,0x00],
  // 0x35 '5'
  [0x00,0xF8,0x80,0xF0,0x08,0x08,0x88,0x70,0x00,0x00],
  // 0x36 '6'
  [0x00,0x30,0x40,0x80,0xF0,0x88,0x88,0x70,0x00,0x00],
  // 0x37 '7'
  [0x00,0xF8,0x08,0x10,0x20,0x40,0x40,0x40,0x00,0x00],
  // 0x38 '8'
  [0x00,0x70,0x88,0x88,0x70,0x88,0x88,0x70,0x00,0x00],
  // 0x39 '9'
  [0x00,0x70,0x88,0x88,0x78,0x08,0x10,0x60,0x00,0x00],
  // 0x3A ':'
  [0x00,0x00,0x00,0x20,0x00,0x00,0x20,0x00,0x00,0x00],
  // 0x3B ';'
  [0x00,0x00,0x00,0x20,0x00,0x00,0x20,0x20,0x40,0x00],
  // 0x3C '<'
  [0x00,0x08,0x10,0x20,0x40,0x20,0x10,0x08,0x00,0x00],
  // 0x3D '='
  [0x00,0x00,0x00,0xF8,0x00,0xF8,0x00,0x00,0x00,0x00],
  // 0x3E '>'
  [0x00,0x40,0x20,0x10,0x08,0x10,0x20,0x40,0x00,0x00],
  // 0x3F '?'
  [0x00,0x70,0x88,0x08,0x10,0x20,0x00,0x20,0x00,0x00],
  // 0x40 '@'
  [0x00,0x70,0x88,0xB8,0xA8,0xB8,0x80,0x70,0x00,0x00],
  // 0x41 'A'
  [0x00,0x70,0x88,0x88,0xF8,0x88,0x88,0x88,0x00,0x00],
  // 0x42 'B'
  [0x00,0xF0,0x88,0x88,0xF0,0x88,0x88,0xF0,0x00,0x00],
  // 0x43 'C'
  [0x00,0x70,0x88,0x80,0x80,0x80,0x88,0x70,0x00,0x00],
  // 0x44 'D'
  [0x00,0xF0,0x88,0x88,0x88,0x88,0x88,0xF0,0x00,0x00],
  // 0x45 'E'
  [0x00,0xF8,0x80,0x80,0xF0,0x80,0x80,0xF8,0x00,0x00],
  // 0x46 'F'
  [0x00,0xF8,0x80,0x80,0xF0,0x80,0x80,0x80,0x00,0x00],
  // 0x47 'G'
  [0x00,0x70,0x88,0x80,0xB8,0x88,0x88,0x70,0x00,0x00],
  // 0x48 'H'
  [0x00,0x88,0x88,0x88,0xF8,0x88,0x88,0x88,0x00,0x00],
  // 0x49 'I'
  [0x00,0x70,0x20,0x20,0x20,0x20,0x20,0x70,0x00,0x00],
  // 0x4A 'J'
  [0x00,0x38,0x10,0x10,0x10,0x10,0x90,0x60,0x00,0x00],
  // 0x4B 'K'
  [0x00,0x88,0x90,0xA0,0xC0,0xA0,0x90,0x88,0x00,0x00],
  // 0x4C 'L'
  [0x00,0x80,0x80,0x80,0x80,0x80,0x80,0xF8,0x00,0x00],
  // 0x4D 'M'
  [0x00,0x88,0xD8,0xA8,0x88,0x88,0x88,0x88,0x00,0x00],
  // 0x4E 'N'
  [0x00,0x88,0xC8,0xA8,0x98,0x88,0x88,0x88,0x00,0x00],
  // 0x4F 'O'
  [0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x00],
  // 0x50 'P'
  [0x00,0xF0,0x88,0x88,0xF0,0x80,0x80,0x80,0x00,0x00],
  // 0x51 'Q'
  [0x00,0x70,0x88,0x88,0x88,0xA8,0x90,0x68,0x00,0x00],
  // 0x52 'R'
  [0x00,0xF0,0x88,0x88,0xF0,0xA0,0x90,0x88,0x00,0x00],
  // 0x53 'S'
  [0x00,0x70,0x88,0x80,0x70,0x08,0x88,0x70,0x00,0x00],
  // 0x54 'T'
  [0x00,0xF8,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00],
  // 0x55 'U'
  [0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x00],
  // 0x56 'V'
  [0x00,0x88,0x88,0x88,0x88,0x50,0x50,0x20,0x00,0x00],
  // 0x57 'W'
  [0x00,0x88,0x88,0x88,0x88,0xA8,0xD8,0x88,0x00,0x00],
  // 0x58 'X'
  [0x00,0x88,0x88,0x50,0x20,0x50,0x88,0x88,0x00,0x00],
  // 0x59 'Y'
  [0x00,0x88,0x88,0x50,0x20,0x20,0x20,0x20,0x00,0x00],
  // 0x5A 'Z'
  [0x00,0xF8,0x08,0x10,0x20,0x40,0x80,0xF8,0x00,0x00],
  // 0x5B '['
  [0x00,0x70,0x40,0x40,0x40,0x40,0x40,0x70,0x00,0x00],
  // 0x5C '\'
  [0x00,0x80,0x80,0x40,0x20,0x10,0x08,0x08,0x00,0x00],
  // 0x5D ']'
  [0x00,0x70,0x10,0x10,0x10,0x10,0x10,0x70,0x00,0x00],
  // 0x5E '^'
  [0x00,0x20,0x50,0x88,0x00,0x00,0x00,0x00,0x00,0x00],
  // 0x5F '_'
  [0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF8,0x00,0x00],
  // 0x60 '`'
  [0x00,0x40,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x00],
  // 0x61 'a'
  [0x00,0x00,0x00,0x70,0x08,0x78,0x88,0x78,0x00,0x00],
  // 0x62 'b'
  [0x00,0x80,0x80,0xF0,0x88,0x88,0x88,0xF0,0x00,0x00],
  // 0x63 'c'
  [0x00,0x00,0x00,0x70,0x88,0x80,0x88,0x70,0x00,0x00],
  // 0x64 'd'
  [0x00,0x08,0x08,0x78,0x88,0x88,0x88,0x78,0x00,0x00],
  // 0x65 'e'
  [0x00,0x00,0x00,0x70,0x88,0xF8,0x80,0x70,0x00,0x00],
  // 0x66 'f'
  [0x00,0x30,0x48,0x40,0xF0,0x40,0x40,0x40,0x00,0x00],
  // 0x67 'g'
  [0x00,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x70,0x00],
  // 0x68 'h'
  [0x00,0x80,0x80,0xF0,0x88,0x88,0x88,0x88,0x00,0x00],
  // 0x69 'i'
  [0x00,0x20,0x00,0x60,0x20,0x20,0x20,0x70,0x00,0x00],
  // 0x6A 'j'
  [0x00,0x10,0x00,0x30,0x10,0x10,0x10,0x90,0x60,0x00],
  // 0x6B 'k'
  [0x00,0x80,0x80,0x90,0xA0,0xC0,0xA0,0x90,0x00,0x00],
  // 0x6C 'l'
  [0x00,0x60,0x20,0x20,0x20,0x20,0x20,0x70,0x00,0x00],
  // 0x6D 'm'
  [0x00,0x00,0x00,0xD0,0xA8,0xA8,0xA8,0x88,0x00,0x00],
  // 0x6E 'n'
  [0x00,0x00,0x00,0xF0,0x88,0x88,0x88,0x88,0x00,0x00],
  // 0x6F 'o'
  [0x00,0x00,0x00,0x70,0x88,0x88,0x88,0x70,0x00,0x00],
  // 0x70 'p'
  [0x00,0x00,0x00,0xF0,0x88,0x88,0xF0,0x80,0x80,0x00],
  // 0x71 'q'
  [0x00,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x08,0x00],
  // 0x72 'r'
  [0x00,0x00,0x00,0xB0,0xC8,0x80,0x80,0x80,0x00,0x00],
  // 0x73 's'
  [0x00,0x00,0x00,0x70,0x80,0x70,0x08,0xF0,0x00,0x00],
  // 0x74 't'
  [0x00,0x40,0x40,0xF0,0x40,0x40,0x48,0x30,0x00,0x00],
  // 0x75 'u'
  [0x00,0x00,0x00,0x88,0x88,0x88,0x88,0x78,0x00,0x00],
  // 0x76 'v'
  [0x00,0x00,0x00,0x88,0x88,0x50,0x50,0x20,0x00,0x00],
  // 0x77 'w'
  [0x00,0x00,0x00,0x88,0xA8,0xA8,0xA8,0x50,0x00,0x00],
  // 0x78 'x'
  [0x00,0x00,0x00,0x88,0x50,0x20,0x50,0x88,0x00,0x00],
  // 0x79 'y'
  [0x00,0x00,0x00,0x88,0x88,0x88,0x78,0x08,0x70,0x00],
  // 0x7A 'z'
  [0x00,0x00,0x00,0xF8,0x10,0x20,0x40,0xF8,0x00,0x00],
  // 0x7B '{'
  [0x00,0x18,0x20,0x20,0xC0,0x20,0x20,0x18,0x00,0x00],
  // 0x7C '|'
  [0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00],
  // 0x7D '}'
  [0x00,0xC0,0x20,0x20,0x18,0x20,0x20,0xC0,0x00,0x00],
  // 0x7E '~'
  [0x00,0x00,0x00,0x48,0xB0,0x00,0x00,0x00,0x00,0x00],
];

/// Draws a single character into an RGBA buffer.
fn draw_char(
  buf: &mut [u8],
  buf_w: u32,
  x: u32,
  y: u32,
  ch: char,
  color: [u8; 3],
) {
  let idx = ch as usize;
  if !(0x20..=0x7E).contains(&idx) {
    return;
  }
  let glyph = &FONT[idx - 0x20];
  for row in 0..FONT_H {
    let py = y as usize + row;
    if py >= (HEIGHT + STATUS_H) as usize {
      continue;
    }
    let bits = glyph[row];
    for col in 0..FONT_W {
      let px = x as usize + col;
      if px >= buf_w as usize {
        continue;
      }
      if bits & (0x80 >> col) != 0 {
        let off = (py * buf_w as usize + px) * 4;
        buf[off] = color[0];
        buf[off + 1] = color[1];
        buf[off + 2] = color[2];
        buf[off + 3] = 255;
      }
    }
  }
}

/// Draws a string into an RGBA buffer.
fn draw_string(
  buf: &mut [u8],
  buf_w: u32,
  x: u32,
  y: u32,
  text: &str,
  color: [u8; 3],
) {
  for (i, ch) in text.chars().enumerate() {
    draw_char(buf, buf_w, x + (i as u32) * FONT_W as u32, y, ch, color);
  }
}

// -----------------------------------------------------------------
// Display modes
// -----------------------------------------------------------------

#[derive(Clone, Copy, PartialEq)]
enum DisplayMode {
  Live,
  Golden,
  Reference,
  RefDiff,
  Diff,
}

impl DisplayMode {
  fn next(self) -> Self {
    match self {
      Self::Live => Self::Golden,
      Self::Golden => Self::Reference,
      Self::Reference => Self::RefDiff,
      Self::RefDiff => Self::Diff,
      Self::Diff => Self::Live,
    }
  }

  fn label(self) -> &'static str {
    match self {
      Self::Live => "Live",
      Self::Golden => "Golden",
      Self::Reference => "Ref",
      Self::RefDiff => "RefDiff",
      Self::Diff => "Diff",
    }
  }
}

// -----------------------------------------------------------------
// Test status
// -----------------------------------------------------------------

#[derive(Clone, Copy, PartialEq)]
enum TestStatus {
  Pass,
  Fail,
  New,
}

impl TestStatus {
  fn label(self) -> &'static str {
    match self {
      Self::Pass => "PASS",
      Self::Fail => "FAIL",
      Self::New => "NEW",
    }
  }

  fn color(self) -> [u8; 3] {
    match self {
      Self::Pass => [0, 200, 0],
      Self::Fail => [220, 40, 40],
      Self::New => [220, 200, 0],
    }
  }
}

// -----------------------------------------------------------------
// Cached per-test state
// -----------------------------------------------------------------

struct CachedTest {
  pixels: Vec<u8>,
  golden: Option<Vec<u8>>,
  reference: Option<Vec<u8>>,
  status: TestStatus,
  rmse: f64,
  ref_rmse: f64,
}

// -----------------------------------------------------------------
// Path helpers
// -----------------------------------------------------------------

fn golden_dir() -> PathBuf {
  PathBuf::from(env!("CARGO_MANIFEST_DIR"))
    .join("tests")
    .join("golden")
}

fn reference_dir() -> PathBuf {
  PathBuf::from(env!("CARGO_MANIFEST_DIR"))
    .join("tests")
    .join("reference")
}

fn output_dir() -> PathBuf {
  PathBuf::from(env!("CARGO_MANIFEST_DIR"))
    .join("tests")
    .join("output")
}

fn load_golden(name: &str) -> Option<Vec<u8>> {
  let path = golden_dir().join(format!("{}.png", name));
  if !path.exists() {
    return None;
  }
  let img = image::open(&path).ok()?.to_rgba8();
  if img.width() != WIDTH || img.height() != HEIGHT {
    return None;
  }
  Some(img.into_raw())
}

fn load_reference(name: &str) -> Option<Vec<u8>> {
  let path = reference_dir().join(format!("{}.png", name));
  if !path.exists() {
    return None;
  }
  let img = image::open(&path).ok()?.to_rgba8();
  Some(img.into_raw())
}

// -----------------------------------------------------------------
// Default thresholds (same as tests/common)
// -----------------------------------------------------------------

const DEFAULT_MAX_RMSE: f64 = 2.0;
const DEFAULT_MAX_DIFF_PCT: f64 = 2.0;

/// Renders a test case and computes its status.
fn render_test(
  tc: &test_cases::TestCase,
) -> CachedTest {
  let mut cap = PlotCapture::new(WIDTH, HEIGHT);
  let fig = cap.figure();
  (tc.setup)(&fig);
  let pixels = cap.render_and_capture();
  let golden = load_golden(tc.name);

  let (status, rmse) = match &golden {
    Some(g) => {
      let r =
        compare::compare_images(&pixels, g, WIDTH, HEIGHT);
      let st = if r.rmse <= DEFAULT_MAX_RMSE
        && r.diff_pct <= DEFAULT_MAX_DIFF_PCT
      {
        TestStatus::Pass
      } else {
        TestStatus::Fail
      };
      (st, r.rmse)
    }
    None => (TestStatus::New, 0.0),
  };

  let reference = load_reference(tc.name);
  let ref_rmse = match &reference {
    Some(r) => {
      // Reference may differ in size; only compare if match.
      let ref_pixels = r.len() as u32 / 4;
      if ref_pixels == WIDTH * HEIGHT {
        compare::compare_images(&pixels, r, WIDTH, HEIGHT)
          .rmse
      } else {
        0.0
      }
    }
    None => 0.0,
  };

  CachedTest { pixels, golden, reference, status, rmse, ref_rmse }
}

/// Refreshes cached state after bless.
fn refresh_after_bless(
  cached: &mut CachedTest,
  name: &str,
) {
  let path =
    golden_dir().join(format!("{}.png", name));
  image::save_buffer(
    &path,
    &cached.pixels,
    WIDTH,
    HEIGHT,
    image::ColorType::Rgba8,
  )
  .expect("Failed to bless golden image");

  cached.golden = Some(cached.pixels.clone());
  cached.status = TestStatus::Pass;
  cached.rmse = 0.0;
}

/// Generates a gnuplot reference PNG for one test case and
/// updates the cached reference + ref_rmse.
fn generate_reference(
  tc: &test_cases::TestCase,
  cached: &mut CachedTest,
) {
  let gnuplot_fig = GnuplotFigure::new();
  let fig = gnuplot_fig.figure();
  (tc.setup)(&fig);
  let ref_dir = reference_dir();
  std::fs::create_dir_all(&ref_dir).ok();
  let path = ref_dir.join(format!("{}.png", tc.name));
  gnuplot_fig.save(path.to_str().unwrap_or(""));
  // Reload the saved PNG to get pixel data at any size,
  // then resize to WIDTH x HEIGHT for comparison.
  cached.reference = load_reference(tc.name);
  cached.ref_rmse = match &cached.reference {
    Some(r) => {
      let ref_pixels = r.len() as u32 / 4;
      if ref_pixels == WIDTH * HEIGHT {
        compare::compare_images(
          &cached.pixels, r, WIDTH, HEIGHT,
        )
        .rmse
      } else {
        0.0
      }
    }
    None => 0.0,
  };
}

/// Builds the composite frame buffer (800x660) with status bar.
fn build_frame(
  cached: &CachedTest,
  mode: DisplayMode,
  idx: usize,
  total: usize,
  name: &str,
) -> Vec<u8> {
  let stride = WIDTH as usize * 4;
  let total_pixels = (WIDTH * WIN_H) as usize * 4;
  let mut buf = vec![0u8; total_pixels];

  // Helper: blit a diff image, draw status bar, return.
  let blit_diff = |buf: &mut Vec<u8>, diff: &[u8]| {
    for row in 0..HEIGHT as usize {
      let src_off = row * stride;
      let dst_off = row * stride;
      buf[dst_off..dst_off + stride]
        .copy_from_slice(&diff[src_off..src_off + stride]);
    }
  };

  // Content area (top 800x600).
  let content = match mode {
    DisplayMode::Live => &cached.pixels,
    DisplayMode::Golden => {
      if let Some(ref g) = cached.golden {
        g
      } else {
        &cached.pixels
      }
    }
    DisplayMode::Reference => {
      if let Some(ref r) = cached.reference {
        r
      } else {
        // Gray placeholder when no reference exists.
        let gray =
          vec![128; (WIDTH * HEIGHT * 4) as usize];
        blit_diff(&mut buf, &gray);
        draw_status_bar(
          &mut buf, idx, total, name, cached, mode,
        );
        return buf;
      }
    }
    DisplayMode::RefDiff => {
      let diff = if let Some(ref r) = cached.reference {
        compare::diff_pixels(&cached.pixels, r)
      } else {
        vec![128; (WIDTH * HEIGHT * 4) as usize]
      };
      blit_diff(&mut buf, &diff);
      draw_status_bar(
        &mut buf, idx, total, name, cached, mode,
      );
      return buf;
    }
    DisplayMode::Diff => {
      let diff = if let Some(ref g) = cached.golden {
        compare::diff_pixels(&cached.pixels, g)
      } else {
        vec![128; (WIDTH * HEIGHT * 4) as usize]
      };
      blit_diff(&mut buf, &diff);
      draw_status_bar(
        &mut buf, idx, total, name, cached, mode,
      );
      return buf;
    }
  };

  for row in 0..HEIGHT as usize {
    let src_off = row * stride;
    let dst_off = row * stride;
    buf[dst_off..dst_off + stride]
      .copy_from_slice(&content[src_off..src_off + stride]);
  }

  draw_status_bar(&mut buf, idx, total, name, cached, mode);
  buf
}

/// Draws the 60px status bar at the bottom of the frame buffer.
fn draw_status_bar(
  buf: &mut [u8],
  idx: usize,
  total: usize,
  name: &str,
  cached: &CachedTest,
  mode: DisplayMode,
) {
  let stride = WIDTH as usize * 4;

  // Fill status bar background (#333333).
  for row in HEIGHT as usize..WIN_H as usize {
    let off = row * stride;
    for col in 0..WIDTH as usize {
      let p = off + col * 4;
      buf[p] = 0x33;
      buf[p + 1] = 0x33;
      buf[p + 2] = 0x33;
      buf[p + 3] = 0xFF;
    }
  }

  let white = [255, 255, 255];
  let gray = [160, 160, 160];
  let y1 = HEIGHT + 10;
  let y2 = HEIGHT + 28;

  // Line 1: [idx/total] name  STATUS  RMSE=x.xx
  let nav = format!("[{}/{}]", idx + 1, total);
  let mut x = 10u32;
  draw_string(buf, WIDTH, x, y1, &nav, white);
  x += (nav.len() as u32 + 1) * FONT_W as u32;

  draw_string(buf, WIDTH, x, y1, name, white);
  x += (name.len() as u32 + 2) * FONT_W as u32;

  draw_string(
    buf, WIDTH, x, y1, cached.status.label(),
    cached.status.color(),
  );
  x += (cached.status.label().len() as u32 + 2)
    * FONT_W as u32;

  let shown_rmse = match mode {
    DisplayMode::Reference | DisplayMode::RefDiff => {
      cached.ref_rmse
    }
    _ => cached.rmse,
  };
  let rmse_str = format!("RMSE={:.2}", shown_rmse);
  draw_string(buf, WIDTH, x, y1, &rmse_str, white);
  x += (rmse_str.len() as u32 + 2) * FONT_W as u32;

  let mode_str = format!("[{}]", mode.label());
  draw_string(buf, WIDTH, x, y1, &mode_str, gray);

  // Line 2: key hints.
  draw_string(
    buf, WIDTH, 10, y2,
    "<-/-> nav  Tab mode  B bless  A all  \
     G gnuplot  R rerender  S save  Q quit",
    gray,
  );
}

// -----------------------------------------------------------------
// Main
// -----------------------------------------------------------------

fn main() -> Result<(), Box<dyn std::error::Error>> {
  let cases = test_cases::all();
  if cases.is_empty() {
    eprintln!("No test cases defined.");
    return Ok(());
  }

  // Pre-render all test cases.
  eprintln!("Pre-rendering {} test cases...", cases.len());
  let mut cached: Vec<CachedTest> = cases
    .iter()
    .enumerate()
    .map(|(i, tc)| {
      eprintln!(
        "  [{}/{}] {}...",
        i + 1,
        cases.len(),
        tc.name
      );
      render_test(tc)
    })
    .collect();

  // Print summary.
  let n_pass =
    cached.iter().filter(|c| c.status == TestStatus::Pass).count();
  let n_fail =
    cached.iter().filter(|c| c.status == TestStatus::Fail).count();
  let n_new =
    cached.iter().filter(|c| c.status == TestStatus::New).count();
  let n_ref =
    cached.iter().filter(|c| c.reference.is_some()).count();
  eprintln!(
    "Summary: {} pass, {} fail, {} new, {} refs",
    n_pass, n_fail, n_new, n_ref,
  );
  if n_ref == 0 {
    eprintln!(
      "Hint: Press G to generate gnuplot references."
    );
  }

  // Start on the first failing test, or the first test.
  let start_idx = cached
    .iter()
    .position(|c| c.status == TestStatus::Fail)
    .or_else(|| {
      cached.iter().position(|c| c.status == TestStatus::New)
    })
    .unwrap_or(0);

  // SDL2 init.
  let sdl = sdl2::init()?;
  let video = sdl.video()?;
  let window = video
    .window("mpl-wgpu Visual Review", WIDTH, WIN_H)
    .position_centered()
    .build()?;

  // wgpu init via SDL2 window.
  let instance = wgpu::Instance::new(wgpu::InstanceDescriptor {
    backends: wgpu::Backends::all(),
    ..Default::default()
  });

  // SDL2's Window is !Send+!Sync so we must use the unsafe
  // surface creation path with raw handles.
  let surface = unsafe {
    let raw_display = window
      .display_handle()
      .expect("display handle")
      .as_raw();
    let raw_window = window
      .window_handle()
      .expect("window handle")
      .as_raw();
    instance.create_surface_unsafe(
      wgpu::SurfaceTargetUnsafe::RawHandle {
        raw_display_handle: raw_display,
        raw_window_handle: raw_window,
      },
    )
  }?;

  let adapter = pollster::block_on(
    instance.request_adapter(&wgpu::RequestAdapterOptions {
      power_preference: wgpu::PowerPreference::default(),
      compatible_surface: Some(&surface),
      force_fallback_adapter: false,
    }),
  )
  .ok_or("No suitable GPU adapter")?;

  let (device, queue) = pollster::block_on(
    adapter.request_device(
      &wgpu::DeviceDescriptor {
        label: Some("DisplayDevice"),
        required_features: wgpu::Features::empty(),
        required_limits: wgpu::Limits::default(),
      },
      None,
    ),
  )?;

  let surface_caps = surface.get_capabilities(&adapter);
  let surface_format = surface_caps
    .formats
    .iter()
    .find(|f| f.is_srgb())
    .copied()
    .unwrap_or(surface_caps.formats[0]);

  let config = wgpu::SurfaceConfiguration {
    usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
    format: surface_format,
    width: WIDTH,
    height: WIN_H,
    present_mode: wgpu::PresentMode::Fifo,
    alpha_mode: surface_caps.alpha_modes[0],
    view_formats: vec![],
    desired_maximum_frame_latency: 2,
  };
  surface.configure(&device, &config);

  // Blit pipeline (full-screen texture draw).
  let blit_texture =
    device.create_texture(&wgpu::TextureDescriptor {
      label: Some("BlitTexture"),
      size: wgpu::Extent3d {
        width: WIDTH,
        height: WIN_H,
        depth_or_array_layers: 1,
      },
      mip_level_count: 1,
      sample_count: 1,
      dimension: wgpu::TextureDimension::D2,
      format: wgpu::TextureFormat::Rgba8UnormSrgb,
      usage: wgpu::TextureUsages::TEXTURE_BINDING
        | wgpu::TextureUsages::COPY_DST,
      view_formats: &[],
    });

  let blit_view = blit_texture
    .create_view(&wgpu::TextureViewDescriptor::default());

  let sampler =
    device.create_sampler(&wgpu::SamplerDescriptor {
      label: Some("BlitSampler"),
      mag_filter: wgpu::FilterMode::Nearest,
      min_filter: wgpu::FilterMode::Nearest,
      ..Default::default()
    });

  let bind_group_layout = device.create_bind_group_layout(
    &wgpu::BindGroupLayoutDescriptor {
      label: Some("BlitBindGroupLayout"),
      entries: &[
        wgpu::BindGroupLayoutEntry {
          binding: 0,
          visibility: wgpu::ShaderStages::FRAGMENT,
          ty: wgpu::BindingType::Texture {
            sample_type: wgpu::TextureSampleType::Float {
              filterable: true,
            },
            view_dimension: wgpu::TextureViewDimension::D2,
            multisampled: false,
          },
          count: None,
        },
        wgpu::BindGroupLayoutEntry {
          binding: 1,
          visibility: wgpu::ShaderStages::FRAGMENT,
          ty: wgpu::BindingType::Sampler(
            wgpu::SamplerBindingType::Filtering,
          ),
          count: None,
        },
      ],
    },
  );

  let bind_group =
    device.create_bind_group(&wgpu::BindGroupDescriptor {
      label: Some("BlitBindGroup"),
      layout: &bind_group_layout,
      entries: &[
        wgpu::BindGroupEntry {
          binding: 0,
          resource: wgpu::BindingResource::TextureView(
            &blit_view,
          ),
        },
        wgpu::BindGroupEntry {
          binding: 1,
          resource: wgpu::BindingResource::Sampler(&sampler),
        },
      ],
    });

  let blit_shader = device.create_shader_module(
    wgpu::ShaderModuleDescriptor {
      label: Some("BlitShader"),
      source: wgpu::ShaderSource::Wgsl(
        r#"
@group(0) @binding(0) var tex: texture_2d<f32>;
@group(0) @binding(1) var samp: sampler;

struct VsOut {
  @builtin(position) pos: vec4<f32>,
  @location(0) uv: vec2<f32>,
};

@vertex
fn vs_main(@builtin(vertex_index) vi: u32) -> VsOut {
  var positions = array<vec2<f32>, 6>(
    vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(-1.0, 1.0),
    vec2(-1.0, 1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
  );
  var uvs = array<vec2<f32>, 6>(
    vec2(0.0, 1.0), vec2(1.0, 1.0), vec2(0.0, 0.0),
    vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(1.0, 0.0),
  );
  var out: VsOut;
  out.pos = vec4(positions[vi], 0.0, 1.0);
  out.uv = uvs[vi];
  return out;
}

@fragment
fn fs_main(in: VsOut) -> @location(0) vec4<f32> {
  return textureSample(tex, samp, in.uv);
}
"#
        .into(),
      ),
    },
  );

  let pipeline_layout = device.create_pipeline_layout(
    &wgpu::PipelineLayoutDescriptor {
      label: Some("BlitPipelineLayout"),
      bind_group_layouts: &[&bind_group_layout],
      push_constant_ranges: &[],
    },
  );

  let blit_pipeline = device.create_render_pipeline(
    &wgpu::RenderPipelineDescriptor {
      label: Some("BlitPipeline"),
      layout: Some(&pipeline_layout),
      vertex: wgpu::VertexState {
        module: &blit_shader,
        entry_point: "vs_main",
        buffers: &[],
        compilation_options:
          wgpu::PipelineCompilationOptions::default(),
      },
      fragment: Some(wgpu::FragmentState {
        module: &blit_shader,
        entry_point: "fs_main",
        targets: &[Some(wgpu::ColorTargetState {
          format: surface_format,
          blend: Some(wgpu::BlendState::REPLACE),
          write_mask: wgpu::ColorWrites::ALL,
        })],
        compilation_options:
          wgpu::PipelineCompilationOptions::default(),
      }),
      primitive: wgpu::PrimitiveState::default(),
      depth_stencil: None,
      multisample: wgpu::MultisampleState::default(),
      multiview: None,
    },
  );

  // State.
  let mut current_idx: usize = start_idx;
  let mut mode = DisplayMode::Live;
  let mut needs_present = true;

  let mut event_pump = sdl.event_pump()?;

  'running: loop {
    for event in event_pump.poll_iter() {
      match event {
        Event::Quit { .. }
        | Event::KeyDown {
          keycode: Some(Keycode::Escape),
          ..
        }
        | Event::KeyDown {
          keycode: Some(Keycode::Q),
          ..
        } => break 'running,

        Event::KeyDown {
          keycode: Some(Keycode::Right),
          ..
        } => {
          current_idx =
            (current_idx + 1) % cases.len();
          needs_present = true;
        }

        Event::KeyDown {
          keycode: Some(Keycode::Left),
          ..
        } => {
          current_idx = if current_idx == 0 {
            cases.len() - 1
          } else {
            current_idx - 1
          };
          needs_present = true;
        }

        // Number keys 1-8 for direct jump.
        Event::KeyDown {
          keycode: Some(kc), ..
        } if matches!(
          kc,
          Keycode::Num1
            | Keycode::Num2
            | Keycode::Num3
            | Keycode::Num4
            | Keycode::Num5
            | Keycode::Num6
            | Keycode::Num7
            | Keycode::Num8
        ) =>
        {
          let n = match kc {
            Keycode::Num1 => 0,
            Keycode::Num2 => 1,
            Keycode::Num3 => 2,
            Keycode::Num4 => 3,
            Keycode::Num5 => 4,
            Keycode::Num6 => 5,
            Keycode::Num7 => 6,
            Keycode::Num8 => 7,
            _ => unreachable!(),
          };
          if n < cases.len() {
            current_idx = n;
            needs_present = true;
          }
        }

        Event::KeyDown {
          keycode: Some(Keycode::Tab),
          ..
        } => {
          mode = mode.next();
          needs_present = true;
        }

        // B: bless current test.
        Event::KeyDown {
          keycode: Some(Keycode::B),
          ..
        } => {
          refresh_after_bless(
            &mut cached[current_idx],
            cases[current_idx].name,
          );
          eprintln!(
            "Blessed: {}",
            cases[current_idx].name,
          );
          needs_present = true;
        }

        // A: bless ALL tests.
        Event::KeyDown {
          keycode: Some(Keycode::A),
          ..
        } => {
          for (i, tc) in cases.iter().enumerate() {
            refresh_after_bless(
              &mut cached[i], tc.name,
            );
            eprintln!("Blessed: {}", tc.name);
          }
          needs_present = true;
        }

        // G: generate gnuplot references for all tests.
        Event::KeyDown {
          keycode: Some(Keycode::G),
          ..
        } => {
          eprintln!(
            "Generating gnuplot references..."
          );
          for (i, tc) in cases.iter().enumerate() {
            eprintln!(
              "  [{}/{}] {}...",
              i + 1, cases.len(), tc.name,
            );
            generate_reference(tc, &mut cached[i]);
          }
          eprintln!("Done generating references.");
          needs_present = true;
        }

        // R: re-render current test.
        Event::KeyDown {
          keycode: Some(Keycode::R),
          ..
        } => {
          eprintln!(
            "Re-rendering {}...",
            cases[current_idx].name,
          );
          cached[current_idx] =
            render_test(&cases[current_idx]);
          needs_present = true;
        }

        // S: save current render to output dir.
        Event::KeyDown {
          keycode: Some(Keycode::S),
          ..
        } => {
          let path = output_dir().join(format!(
            "{}_screenshot.png",
            cases[current_idx].name,
          ));
          image::save_buffer(
            &path,
            &cached[current_idx].pixels,
            WIDTH,
            HEIGHT,
            image::ColorType::Rgba8,
          )
          .ok();
          eprintln!("Saved: {}", path.display());
        }

        _ => {}
      }
    }

    if needs_present {
      needs_present = false;

      let frame_buf = build_frame(
        &cached[current_idx],
        mode,
        current_idx,
        cases.len(),
        cases[current_idx].name,
      );

      // Upload to blit texture.
      queue.write_texture(
        wgpu::ImageCopyTexture {
          texture: &blit_texture,
          mip_level: 0,
          origin: wgpu::Origin3d::ZERO,
          aspect: wgpu::TextureAspect::All,
        },
        &frame_buf,
        wgpu::ImageDataLayout {
          offset: 0,
          bytes_per_row: Some(WIDTH * 4),
          rows_per_image: Some(WIN_H),
        },
        wgpu::Extent3d {
          width: WIDTH,
          height: WIN_H,
          depth_or_array_layers: 1,
        },
      );

      // Present.
      let frame = surface.get_current_texture()?;
      let view = frame.texture.create_view(
        &wgpu::TextureViewDescriptor::default(),
      );
      let mut encoder = device.create_command_encoder(
        &wgpu::CommandEncoderDescriptor {
          label: Some("BlitEncoder"),
        },
      );
      {
        let mut rp = encoder.begin_render_pass(
          &wgpu::RenderPassDescriptor {
            label: Some("BlitPass"),
            color_attachments: &[Some(
              wgpu::RenderPassColorAttachment {
                view: &view,
                resolve_target: None,
                ops: wgpu::Operations {
                  load: wgpu::LoadOp::Clear(
                    wgpu::Color::BLACK,
                  ),
                  store: wgpu::StoreOp::Store,
                },
              },
            )],
            depth_stencil_attachment: None,
            ..Default::default()
          },
        );
        rp.set_pipeline(&blit_pipeline);
        rp.set_bind_group(0, &bind_group, &[]);
        rp.draw(0..6, 0..1);
      }
      queue.submit(std::iter::once(encoder.finish()));
      frame.present();
    }

    // ~60fps sleep.
    std::thread::sleep(std::time::Duration::from_millis(16));
  }

  Ok(())
}
