// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

#include "test_cases.h"
#include <cmath>

namespace mpl_wgpu {

namespace plt = matplot;

namespace {

void TestLinePlot(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  auto ax = f->add_axes();
  auto x = plt::linspace(0, 10, 100);
  auto y = plt::transform(
      x, [](double v) { return std::sin(v); });
  ax->plot(x, y)->line_width(3);
  ax->title("Sine Wave");
  ax->xlabel("Time (s)");
  ax->ylabel("Amplitude");
}

void TestScatterPlot(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  auto ax = f->add_axes();
  // Deterministic pseudo-random data (LCG) so gnuplot
  // reference and wgpu render use identical points.
  std::vector<double> x(100), y(100), s(100), c(100);
  uint32_t seed = 12345;
  auto next = [&]() -> double {
    seed = seed * 1103515245u + 12345u;
    return static_cast<double>(
               (seed >> 16) & 0x7FFF) / 32767.0;
  };
  for (int i = 0; i < 100; ++i) {
    double u1 = std::max(1e-10, next());
    double u2 = next();
    x[i] = std::sqrt(-2.0 * std::log(u1))
           * std::cos(2.0 * M_PI * u2);
    u1 = std::max(1e-10, next());
    u2 = next();
    y[i] = std::sqrt(-2.0 * std::log(u1))
           * std::cos(2.0 * M_PI * u2);
    s[i] = 10.0 + 40.0 * next();
    c[i] = x[i];
  }
  ax->scatter(x, y, s, c);
  ax->title("Random Scatter");
}

void TestBarChart(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  auto ax = f->add_axes();
  std::vector<std::string> x = {"A", "B", "C", "D", "E"};
  std::vector<double> y = {10, 25, 15, 30, 8};
  ax->bar(y);
  ax->x_axis().ticklabels(x);
  ax->title("Bar Chart");
}

void TestSubplot(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  std::array<float, 4> pos1 = {
      0.13f, 0.11f, 0.775f, 0.34f};
  auto ax1 =
      std::make_shared<matplot::axes_type>(f, pos1);
  f->add_axes(ax1, false, true);
  ax1->plot(plt::iota(1, 10));
  ax1->title("Subplot 1 (Bottom)");
  ax1->box(true);
  std::array<float, 4> pos2 = {
      0.13f, 0.58f, 0.775f, 0.34f};
  auto ax2 =
      std::make_shared<matplot::axes_type>(f, pos2);
  f->add_axes(ax2, false, true);
  ax2->bar(std::vector<double>{5, 3, 7, 2});
  ax2->title("Subplot 2 (Top)");
  ax2->box(true);
}

void Test3DLine(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  auto ax = f->add_axes();
  auto t = plt::linspace(0, 10 * 3.14, 200);
  auto x = plt::transform(
      t, [](double v) { return std::sin(v); });
  auto y = plt::transform(
      t, [](double v) { return std::cos(v); });
  auto z = t;
  ax->plot3(x, y, z)->line_width(2);
  ax->title("3D Helix");
}

void TestStemPlot(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  auto ax = f->add_axes();
  auto x = plt::linspace(0, 2 * 3.14, 20);
  auto y = plt::transform(
      x, [](double v) { return std::cos(v); });
  ax->stem(x, y);
  ax->title("Stem Plot");
}

void TestAreaPlot(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  auto ax = f->add_axes();
  std::vector<double> y = {1, 3, 2, 5, 4, 8};
  ax->area(y);
  ax->title("Area Plot");
}

void TestScatterLabels(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  std::array<float, 4> pos = {
      0.13f, 0.11f, 0.775f, 0.815f};
  auto ax =
      std::make_shared<matplot::axes_type>(f, pos);
  f->add_axes(ax, false, true);
  std::vector<double> x = {15, 30, 45, 60, 75};
  std::vector<double> y = {10, 25, 15, 35, 20};
  auto l = ax->plot(x, y, "o");
  l->marker_face_color("blue");
  l->marker_size(10);
  std::vector<std::string> labels = {
      "Point A", "Point B", "Point C",
      "Point D", "Point E"};
  for (size_t i = 0; i < x.size(); ++i) {
    ax->text(x[i] + 1, y[i] + 1, labels[i]);
  }
  ax->title("Scatter with Labels");
  ax->xlabel("X Value");
  ax->ylabel("Y Value");
  ax->box(true);
}

void TestHistogram(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  auto ax = f->add_axes();
  std::vector<double> data;
  for (int i = 0; i < 100; ++i) {
    data.push_back(
        std::sin(i * 0.3) * 5 + 10 + (i % 7) - 3);
  }
  ax->hist(data);
  ax->title("Histogram");
  ax->xlabel("Value");
  ax->ylabel("Frequency");
}

void TestStairs(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  auto ax = f->add_axes();
  std::vector<double> x = {0, 1, 2, 3, 4, 5, 6, 7};
  std::vector<double> y = {3, 4, 2, 5, 3, 6, 4, 7};
  ax->stairs(x, y);
  ax->title("Stairs (Step) Plot");
  ax->xlabel("X");
  ax->ylabel("Y");
}

void TestLogLog(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  auto ax = f->add_axes();
  std::vector<double> x = {1, 10, 100, 1000, 10000};
  std::vector<double> y = {1, 10, 100, 1000, 10000};
  ax->loglog(x, y);
  ax->title("Log-Log Plot");
  ax->xlabel("X (log)");
  ax->ylabel("Y (log)");
}

void TestErrorBar(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  auto ax = f->add_axes();
  std::vector<double> x = {1, 2, 3, 4, 5};
  std::vector<double> y = {2, 4, 3, 5, 4};
  std::vector<double> err = {0.5, 0.3, 0.4, 0.6, 0.3};
  ax->errorbar(x, y, err);
  ax->title("Error Bar Plot");
  ax->xlabel("X");
  ax->ylabel("Y with Error");
}

void TestPolarPlot(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  std::vector<double> theta, rho;
  for (double t = 0; t <= 2 * M_PI; t += 0.1) {
    theta.push_back(t);
    rho.push_back(1 + std::cos(t));
  }
  auto ax = f->add_axes(true);
  ax->polarplot(theta, rho);
  ax->title("Polar (Cardioid)");
}

void TestPieChart(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  auto ax = f->add_axes();
  std::vector<double> values = {35, 25, 20, 15, 5};
  ax->pie(values);
  ax->title("Pie Chart");
}

void TestHeatmap(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  auto ax = f->add_axes();
  std::vector<std::vector<double>> data(
      10, std::vector<double>(10));
  for (size_t i = 0; i < 10; ++i) {
    for (size_t j = 0; j < 10; ++j) {
      data[i][j] = std::sin(i * 0.5) * std::cos(j * 0.5);
    }
  }
  ax->heatmap(data);
  ax->title("Heatmap");
}

void TestContour(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  auto ax = f->add_axes();
  auto [X, Y] = matplot::meshgrid(
      matplot::linspace(-2, 2, 20),
      matplot::linspace(-2, 2, 20));
  auto Z = matplot::transform(
      X, Y,
      [](double x, double y) {
        return std::exp(-(x * x + y * y));
      });
  ax->contour(X, Y, Z);
  ax->title("Contour Plot");
}

void TestBoxPlot(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  auto ax = f->add_axes();
  std::vector<std::vector<double>> data = {
      {1, 2, 3, 4, 5, 6, 7},
      {2, 3, 4, 5, 6, 7, 8},
      {3, 4, 5, 6, 7, 8, 10}};
  ax->boxplot(data);
  ax->title("Box Plot");
}

void TestQuiver(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  auto ax = f->add_axes();
  std::vector<double> x = {0, 1, 2, 0, 1, 2, 0, 1, 2};
  std::vector<double> y = {0, 0, 0, 1, 1, 1, 2, 2, 2};
  std::vector<double> u = {1, 1, 1, 0, 0, 0, -1, -1, -1};
  std::vector<double> v = {0, 1, 0, 1, 0, -1, 0, -1, 0};
  ax->quiver(x, y, u, v);
  ax->title("Vector Field (Quiver)");
}

void TestMultiLine(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  auto ax = f->add_axes();
  auto x = matplot::linspace(0, 2 * M_PI, 200);
  ax->plot(x, matplot::transform(
      x, [](double v) { return std::sin(v); }));
  ax->hold(true);
  ax->plot(x, matplot::transform(
      x, [](double v) { return std::cos(v); }));
  ax->plot(x, matplot::transform(
      x, [](double v) { return std::sin(2 * v); }));
  ax->hold(false);
  ax->title("Multiple Lines");
  ax->xlabel("X");
  ax->ylabel("Y");
}

void TestScatter3D(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  auto ax = f->add_axes();
  std::vector<double> x, y, z;
  for (int i = 0; i < 50; ++i) {
    double t = i * 0.2;
    x.push_back(
        std::cos(t) * (1 + 0.3 * std::sin(t * 3)));
    y.push_back(
        std::sin(t) * (1 + 0.3 * std::cos(t * 2)));
    z.push_back(t * 0.1);
  }
  ax->scatter3(x, y, z);
  ax->title("3D Scatter");
}

void TestStem3D(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  auto ax = f->add_axes();
  std::vector<double> x, y, z;
  for (int i = 0; i < 20; ++i) {
    double t = i * 0.3;
    x.push_back(std::cos(t));
    y.push_back(std::sin(t));
    z.push_back(t * 0.2);
  }
  ax->stem3(x, y, z);
  ax->title("3D Stem");
}

void TestSemiLogX(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  auto ax = f->add_axes();
  std::vector<double> x = {1, 10, 100, 1000, 10000};
  std::vector<double> y = {1, 2, 3, 4, 5};
  ax->semilogx(x, y);
  ax->title("Semi-Log X");
}

void TestSemiLogY(matplot::figure_handle f) {
  if (!f) return;
  f->color({0., 1.f, 1.f, 1.f});
  f->children(
      std::vector<std::shared_ptr<matplot::axes_type>>{});
  auto ax = f->add_axes();
  std::vector<double> x = {1, 2, 3, 4, 5};
  std::vector<double> y = {1, 10, 100, 1000, 10000};
  ax->semilogy(x, y);
  ax->title("Semi-Log Y");
}

}  // namespace

std::vector<TestCaseEntry> GetAllTestCases() {
  return {
      {"line_plot", TestLinePlot},
      {"scatter_plot", TestScatterPlot},
      {"bar_chart", TestBarChart},
      {"multi_line", TestMultiLine},
      {"histogram", TestHistogram},
      {"subplots", TestSubplot},
      {"helix_3d", Test3DLine},
      {"heatmap", TestHeatmap},
      {"stem_plot", TestStemPlot},
      {"area_plot", TestAreaPlot},
      {"scatter_labels", TestScatterLabels},
      {"stairs_plot", TestStairs},
      {"log_log", TestLogLog},
      {"error_bar", TestErrorBar},
      {"polar_plot", TestPolarPlot},
      {"pie_chart", TestPieChart},
      {"contour", TestContour},
      {"box_plot", TestBoxPlot},
      {"quiver", TestQuiver},
      {"scatter_3d", TestScatter3D},
      {"stem_3d", TestStem3D},
      {"semi_log_x", TestSemiLogX},
      {"semi_log_y", TestSemiLogY},
  };
}

}  // namespace mpl_wgpu
