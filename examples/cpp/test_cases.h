// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

/// Shared test case registry for visual regression testing.

#ifndef MPL_WGPU_EXAMPLE_TEST_CASES_H_
#define MPL_WGPU_EXAMPLE_TEST_CASES_H_

#include <functional>
#include <string>
#include <vector>
#include <matplot/matplot.h>

namespace mpl_wgpu {

/// A named test case with a matplot++ figure setup function.
struct TestCaseEntry {
  std::string name;
  std::function<void(matplot::figure_handle)> setup;
};

/// Returns all registered visual test cases.
std::vector<TestCaseEntry> GetAllTestCases();

}  // namespace mpl_wgpu

#endif  // MPL_WGPU_EXAMPLE_TEST_CASES_H_
