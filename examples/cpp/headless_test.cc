// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

// Headless visual comparison tool for mpl-wgpu.
//
// Renders each test case with both the wgpu backend and matplot++'s
// gnuplot backend, then compares them pixel-by-pixel. This lets you
// iterate on the wgpu renderer to match matplot++ output without
// needing a display or SDL2.
//
// Usage:
//   headless_test [--filter PATTERN] [--gen-refs] [--bless]
//
// Modes:
//   (default)    Compare wgpu renders against gnuplot references.
//                Generates refs automatically if missing.
//   --gen-refs   Regenerate all gnuplot references, then compare.
//   --bless      Save wgpu renders as golden regression images.
//
// Environment:
//   BLESS=1      Same as --bless.
//
// Output:
//   tests/reference/<name>.png   gnuplot reference
//   tests/output/<name>.png      wgpu render (always saved)
//   tests/output/<name>_diff.png red-highlight diff vs reference
//
// Exit codes:
//   0  All rendered (bless/gen-refs mode)
//   1  Fatal error
//   2  No tests matched filter

#include "test_cases.h"
#include "image_utils.h"
#include "../../src/backend/primitive_renderer.h"
#include "../../src/backend/frame_capture.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using namespace mpl_wgpu;
namespace plt = matplot;

static constexpr uint32_t kWidth = 800;
static constexpr uint32_t kHeight = 600;

// -----------------------------------------------------------------
// Path helpers
// -----------------------------------------------------------------

static std::string ProjectDir() {
  namespace fs = std::filesystem;
  for (auto dir = fs::current_path();
       dir != dir.root_path();
       dir = dir.parent_path()) {
    if (fs::exists(dir / "tests" / "golden")) {
      return dir.string();
    }
  }
  return fs::current_path().string();
}

static std::string RefPath(const std::string& base,
                           const std::string& name) {
  return base + "/tests/reference/" + name + ".png";
}

static std::string GoldenPath(const std::string& base,
                              const std::string& name) {
  return base + "/tests/golden/" + name + ".png";
}

static std::string OutputPath(const std::string& base,
                              const std::string& name) {
  return base + "/tests/output/" + name + ".png";
}

// -----------------------------------------------------------------
// Generate a gnuplot reference PNG for one test case.
// -----------------------------------------------------------------

static bool GenerateRef(const std::string& project_dir,
                        const TestCaseEntry& tc) {
  auto path = RefPath(project_dir, tc.name);
  std::filesystem::create_directories(
      std::filesystem::path(path).parent_path());

  // matplot++ figure with default gnuplot backend.
  // Match the wgpu render size to avoid nearest-neighbor
  // resize artifacts in the comparison.
  auto gfig = plt::figure(true);
  gfig->size(kWidth, kHeight);
  tc.setup(gfig);
  gfig->save(path);

  return std::filesystem::exists(path);
}

// -----------------------------------------------------------------
// Offscreen render a test case with wgpu.
// -----------------------------------------------------------------

static std::vector<uint8_t> RenderTestCase(
    PrimitiveRenderer& renderer,
    matplot::backend::WgpuBackend& backend,
    matplot::figure_handle fig,
    FrameCapture& capture,
    wgpu::Device device, wgpu::Queue queue,
    const TestCaseEntry& tc) {
  fig->backend(
      std::shared_ptr<matplot::backend::WgpuBackend>(
          &backend, [](auto*) {}));
  fig->size(kWidth, kHeight);
  tc.setup(fig);

  renderer.Clear();
  fig->draw();
  renderer.Prepare(queue);

  wgpu::CommandEncoder encoder =
      device.CreateCommandEncoder();
  auto pass_desc = capture.BeginPassDescriptor();
  auto pass = encoder.BeginRenderPass(&pass_desc);
  renderer.Render(pass);
  pass.End();

  return capture.CopyAndReadback(encoder, queue, device);
}

// -----------------------------------------------------------------
// Main
// -----------------------------------------------------------------

int main(int argc, char* argv[]) {
  // Parse CLI args.
  std::string filter;
  bool gen_refs = false;
  bool bless = false;

  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--filter") == 0
        && i + 1 < argc) {
      filter = argv[++i];
    } else if (std::strcmp(argv[i], "--gen-refs") == 0) {
      gen_refs = true;
    } else if (std::strcmp(argv[i], "--bless") == 0) {
      bless = true;
    }
  }

  const char* bless_env = std::getenv("BLESS");
  if (bless_env && std::strcmp(bless_env, "1") == 0) {
    bless = true;
  }

  try {
    auto all_tests = GetAllTestCases();

    // Apply filter.
    std::vector<TestCaseEntry> tests;
    for (auto& tc : all_tests) {
      if (filter.empty()
          || tc.name.find(filter) != std::string::npos) {
        tests.push_back(std::move(tc));
      }
    }

    if (tests.empty()) {
      std::cerr << "No tests matched filter: " << filter
                << std::endl;
      return 2;
    }

    std::string project_dir = ProjectDir();

    // Generate gnuplot references if requested or missing.
    if (gen_refs) {
      std::cerr << "Generating gnuplot references..."
                << std::endl;
    }
    for (size_t i = 0; i < tests.size(); ++i) {
      auto rp = RefPath(project_dir, tests[i].name);
      bool need = gen_refs || !std::filesystem::exists(rp);
      if (!need) continue;
      std::cerr << "  ref [" << (i + 1) << "/"
                << tests.size() << "] "
                << tests[i].name << std::endl;
      if (!GenerateRef(project_dir, tests[i])) {
        std::cerr << "    WARNING: gnuplot ref failed"
                  << std::endl;
      }
    }

    // Headless wgpu init (no surface).
    WGPUInstanceDescriptor instance_desc{};
    auto instance = wgpu::CreateInstance(&instance_desc);
    if (!instance) {
      std::cerr << "Failed to create WebGPU instance"
                << std::endl;
      return 1;
    }

    WGPURequestAdapterOptions adapter_opts{};
    adapter_opts.compatibleSurface = nullptr;
    auto adapter = instance.RequestAdapter(&adapter_opts);
    if (!adapter) {
      std::cerr << "Failed to request WebGPU adapter"
                << std::endl;
      return 1;
    }

    WGPUDeviceDescriptor device_desc{};
    device_desc.label = "mpl-wgpu headless device";
    auto device = adapter.RequestDevice(&device_desc);
    if (!device) {
      std::cerr << "Failed to request WebGPU device"
                << std::endl;
      return 1;
    }

    auto queue = device.GetQueue();

    // Offscreen renderer at RGBA8Unorm for capture.
    auto renderer = std::make_shared<PrimitiveRenderer>(
        device, wgpu::TextureFormat::RGBA8Unorm,
        kWidth, kHeight);
    auto backend =
        std::make_shared<matplot::backend::WgpuBackend>(
            renderer);
    backend->width(kWidth);
    backend->height(kHeight);
    backend->set_render_size(kWidth, kHeight);

    auto fig = plt::figure(true);
    fig->backend(backend);
    fig->size(kWidth, kHeight);

    FrameCapture capture(device, kWidth, kHeight);

    std::cerr << "Rendering " << tests.size()
              << " tests with wgpu..." << std::endl;

    // Column header.
    std::printf("%-22s  %8s  %8s  %s\n",
                "TEST", "RMSE", "DIFF%", "REF");

    double worst_rmse = 0;
    double total_rmse = 0;

    for (size_t i = 0; i < tests.size(); ++i) {
      const auto& tc = tests[i];

      auto pixels = RenderTestCase(
          *renderer, *backend, fig, capture,
          device, queue, tc);

      // Always save the wgpu render.
      SavePng(OutputPath(project_dir, tc.name),
              pixels.data(), kWidth, kHeight);

      if (bless) {
        SavePng(GoldenPath(project_dir, tc.name),
                pixels.data(), kWidth, kHeight);
        std::printf("%-22s  (blessed)\n",
                    tc.name.c_str());
        continue;
      }

      // Load gnuplot reference.
      auto ref = LoadPng(RefPath(project_dir, tc.name),
                         kWidth, kHeight);
      if (!ref) {
        std::printf("%-22s  %8s  %8s  NO REF\n",
                    tc.name.c_str(), "-", "-");
        continue;
      }

      auto cmp = CompareImages(
          pixels.data(), ref->pixels.data(),
          kWidth, kHeight);

      total_rmse += cmp.rmse;
      if (cmp.rmse > worst_rmse) {
        worst_rmse = cmp.rmse;
      }

      // Save diff image.
      auto diff = DiffPixels(
          pixels.data(), ref->pixels.data(),
          kWidth, kHeight);
      SavePng(OutputPath(project_dir, tc.name + "_diff"),
              diff.data(), kWidth, kHeight);

      std::printf("%-22s  %8.2f  %7.2f%%  %s\n",
                  tc.name.c_str(), cmp.rmse, cmp.diff_pct,
                  RefPath(project_dir, tc.name).c_str());
    }

    if (!bless) {
      double avg_rmse = total_rmse / tests.size();
      std::printf("\n--- Summary ---\n");
      std::printf("Tests: %zu  Avg RMSE: %.2f"
                  "  Worst RMSE: %.2f\n",
                  tests.size(), avg_rmse, worst_rmse);
      std::printf("Output: %s/tests/output/\n",
                  project_dir.c_str());
    }

    return 0;

  } catch (const std::exception& e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return 1;
  }
}
