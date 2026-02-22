// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

/// PNG I/O and image comparison utilities for visual regression.

#ifndef MPL_WGPU_EXAMPLE_IMAGE_UTILS_H_
#define MPL_WGPU_EXAMPLE_IMAGE_UTILS_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mpl_wgpu {

/// RGBA pixel buffer with dimensions.
struct Image {
  std::vector<uint8_t> pixels;
  uint32_t width;
  uint32_t height;
};

/// Result of comparing two images.
struct CompareResult {
  double rmse;
  double diff_pct;
};

/// Loads a PNG file as RGBA pixels. If the image dimensions differ
/// from expected_w x expected_h, performs nearest-neighbor resize.
/// Returns nullopt if the file doesn't exist or can't be read.
std::optional<Image> LoadPng(const std::string& path,
                             uint32_t expected_w,
                             uint32_t expected_h);

/// Saves RGBA pixels as a PNG file. Returns true on success.
bool SavePng(const std::string& path,
             const uint8_t* pixels,
             uint32_t width, uint32_t height);

/// Compares two RGBA images pixel-by-pixel.
/// Returns RMSE and percentage of differing pixels (channel
/// diff > 2 threshold, matching the Rust compare module).
CompareResult CompareImages(const uint8_t* a,
                            const uint8_t* b,
                            uint32_t width,
                            uint32_t height);

/// Generates a red-highlight diff visualization.
/// Pixels that differ get red overlay; matching pixels are dimmed.
std::vector<uint8_t> DiffPixels(const uint8_t* a,
                                const uint8_t* b,
                                uint32_t width,
                                uint32_t height);

}  // namespace mpl_wgpu

#endif  // MPL_WGPU_EXAMPLE_IMAGE_UTILS_H_
