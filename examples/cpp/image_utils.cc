// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

#include "image_utils.h"
#include "../../src/backend/vendor/stb_image.h"
#include "../../src/backend/vendor/stb_image_write.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>

namespace mpl_wgpu {

namespace {

// Per-channel soft threshold matching compare.rs:10.
constexpr int kSoftThreshold = 5;

// Nearest-neighbor resize of RGBA image.
std::vector<uint8_t> ResizeNearest(
    const uint8_t* src, uint32_t src_w, uint32_t src_h,
    uint32_t dst_w, uint32_t dst_h) {
  std::vector<uint8_t> out(
      static_cast<size_t>(dst_w) * dst_h * 4);
  for (uint32_t y = 0; y < dst_h; ++y) {
    uint32_t sy = y * src_h / dst_h;
    for (uint32_t x = 0; x < dst_w; ++x) {
      uint32_t sx = x * src_w / dst_w;
      size_t src_off =
          (static_cast<size_t>(sy) * src_w + sx) * 4;
      size_t dst_off =
          (static_cast<size_t>(y) * dst_w + x) * 4;
      std::memcpy(&out[dst_off], &src[src_off], 4);
    }
  }
  return out;
}

}  // namespace

std::optional<Image> LoadPng(const std::string& path,
                             uint32_t expected_w,
                             uint32_t expected_h) {
  if (!std::filesystem::exists(path)) {
    return std::nullopt;
  }
  int w, h, channels;
  // Force RGBA output.
  unsigned char* data =
      stbi_load(path.c_str(), &w, &h, &channels, 4);
  if (!data) {
    return std::nullopt;
  }

  Image img;
  if (static_cast<uint32_t>(w) == expected_w
      && static_cast<uint32_t>(h) == expected_h) {
    size_t sz = static_cast<size_t>(w) * h * 4;
    img.pixels.assign(data, data + sz);
  } else {
    img.pixels = ResizeNearest(
        data, static_cast<uint32_t>(w),
        static_cast<uint32_t>(h), expected_w, expected_h);
  }
  stbi_image_free(data);

  img.width = expected_w;
  img.height = expected_h;
  return img;
}

bool SavePng(const std::string& path,
             const uint8_t* pixels,
             uint32_t width, uint32_t height) {
  // Ensure parent directory exists.
  auto parent = std::filesystem::path(path).parent_path();
  if (!parent.empty()) {
    std::filesystem::create_directories(parent);
  }
  int stride = static_cast<int>(width * 4);
  return stbi_write_png(path.c_str(),
                        static_cast<int>(width),
                        static_cast<int>(height),
                        4, pixels, stride) != 0;
}

CompareResult CompareImages(const uint8_t* a,
                            const uint8_t* b,
                            uint32_t width,
                            uint32_t height) {
  size_t pixel_count =
      static_cast<size_t>(width) * height;
  double sum_sq = 0.0;
  size_t diff_count = 0;

  for (size_t i = 0; i < pixel_count; ++i) {
    size_t off = i * 4;
    bool pixel_exceeds = false;
    for (int c = 0; c < 4; ++c) {
      int d = std::abs(
          static_cast<int>(a[off + c])
          - static_cast<int>(b[off + c]));
      sum_sq += static_cast<double>(d) * d;
      if (d > kSoftThreshold) {
        pixel_exceeds = true;
      }
    }
    if (pixel_exceeds) {
      ++diff_count;
    }
  }

  double channel_count =
      static_cast<double>(pixel_count * 4);
  CompareResult result;
  result.rmse = std::sqrt(sum_sq / channel_count);
  result.diff_pct = (static_cast<double>(diff_count)
                     / static_cast<double>(pixel_count))
                    * 100.0;
  return result;
}

std::vector<uint8_t> DiffPixels(const uint8_t* a,
                                const uint8_t* b,
                                uint32_t width,
                                uint32_t height) {
  size_t pixel_count =
      static_cast<size_t>(width) * height;
  std::vector<uint8_t> out(pixel_count * 4);

  for (size_t i = 0; i < pixel_count; ++i) {
    size_t off = i * 4;
    int dr = std::abs(static_cast<int>(a[off])
                      - static_cast<int>(b[off]));
    int dg = std::abs(static_cast<int>(a[off + 1])
                      - static_cast<int>(b[off + 1]));
    int db = std::abs(static_cast<int>(a[off + 2])
                      - static_cast<int>(b[off + 2]));
    int max_d = std::max({dr, dg, db});

    if (max_d <= kSoftThreshold) {
      // Match: dim green.
      out[off] = 0;
      out[off + 1] = 128;
      out[off + 2] = 0;
    } else {
      // Differ: red, amplified 10x.
      uint8_t intensity =
          static_cast<uint8_t>(std::min(max_d * 10, 255));
      out[off] = intensity;
      out[off + 1] = 0;
      out[off + 2] = 0;
    }
    out[off + 3] = 255;
  }
  return out;
}

}  // namespace mpl_wgpu
