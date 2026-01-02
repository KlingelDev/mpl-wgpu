#include "window.h"
#include "../../src/backend/primitive_renderer.h"
#include <matplot/matplot.h>
#include <iostream>
#include <vector>
#include <functional>
#include <cmath>

using namespace mpl_wgpu;
namespace plt = matplot;

// --- Test Cases ---

void TestLinePlot(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    auto ax = f->add_axes();
    
    std::vector<double> x = plt::linspace(0, 10, 100);
    std::vector<double> y = plt::transform(x, [](double v) { return std::sin(v); });
    ax->plot(x, y)->line_width(3);
    ax->title("Sine Wave");
    ax->xlabel("Time (s)");
    ax->ylabel("Amplitude");
    // Ensure colors are visible on dark/light
    ax->title_color({0,0,0,1}); 
    ax->x_axis().label_color({0,0,0,1});
    ax->y_axis().label_color({0,0,0,1});
}

void TestScatterPlot(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    auto ax = f->add_axes();
    
    auto x = plt::randn(100, 0, 1);
    auto y = plt::randn(100, 0, 1);
    auto s = plt::rand(100, 10, 50); // Sizes
    auto c = x; // Colors mapped to x
    
    ax->scatter(x, y, s, c);
    ax->title("Random Scatter");
    // ax->colorbar(); // Colorbar might need ax context? 
    // plt::colorbar() uses gca. ax->colorbar() might not exist on axes?
    // Checking headers, axes_type might not have colorbar directly?
    // For now skip colorbar or use plt::colorbar(ax)?
    
    ax->title_color({0,0,0,1});
    ax->x_axis().label_color({0,0,0,1});
    ax->y_axis().label_color({0,0,0,1});
}

void TestBarChart(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    auto ax = f->add_axes();
    
    std::vector<std::string> x = {"A", "B", "C", "D", "E"};
    std::vector<double> y = {10, 25, 15, 30, 8};
    ax->bar(y);
    ax->x_axis().ticklabels(x);
    ax->title("Bar Chart");
    
    ax->title_color({0,0,0,1});
    ax->x_axis().label_color({0,0,0,1});
    ax->y_axis().label_color({0,0,0,1});
}

void TestSubplot(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    
    // Use explicit positioning to avoid subplot indexing ambiguity/bugs
    // Bottom Plot
    std::array<float, 4> pos1 = {0.13f, 0.11f, 0.775f, 0.34f};
    auto ax1 = std::make_shared<matplot::axes_type>(f, pos1);
    f->add_axes(ax1, false, true);
    ax1->plot(plt::iota(1, 10)); // Sine/Line
    ax1->title("Subplot 1 (Bottom)");
    ax1->title_color({0,0,0,1});
    ax1->box(true);

    // Top Plot
    std::array<float, 4> pos2 = {0.13f, 0.58f, 0.775f, 0.34f};
    auto ax2 = std::make_shared<matplot::axes_type>(f, pos2);
    f->add_axes(ax2, false, true);
    ax2->bar(std::vector<double>{5, 3, 7, 2});
    ax2->title("Subplot 2 (Top)");
    ax2->title_color({0,0,0,1});
    ax2->box(true);
    ax2->bar(std::vector<double>{5, 3, 7, 2});
    ax2->title("Subplot 2 (Top)");
    ax2->title_color({0,0,0,1});
    ax2->box(true);
}

void Test3DLine(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    auto ax = f->add_axes();
    
    std::vector<double> t = plt::linspace(0, 10*3.14, 200);
    std::vector<double> x = plt::transform(t, [](double v) { return std::sin(v); });
    std::vector<double> y = plt::transform(t, [](double v) { return std::cos(v); });
    std::vector<double> z = t;
    
    ax->plot3(x, y, z)->line_width(2);
    ax->title("3D Helix");
    ax->title_color({0,0,0,1});
}

void TestStemPlot(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    auto ax = f->add_axes();
    
    std::vector<double> x = plt::linspace(0, 2*3.14, 20);
    std::vector<double> y = plt::transform(x, [](double v) { return std::cos(v); });
    
    ax->stem(x, y);
    ax->title("Stem Plot");
    ax->title_color({0,0,0,1});
    ax->x_axis().label_color({0,0,0,1});
    ax->y_axis().label_color({0,0,0,1});
}

void TestAreaPlot(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    auto ax = f->add_axes();
    
    std::vector<double> y = {1, 3, 2, 5, 4, 8};
    ax->area(y);
    ax->title("Area Plot");
    ax->title_color({0,0,0,1});
    ax->x_axis().label_color({0,0,0,1});
    ax->y_axis().label_color({0,0,0,1});
}

// Main Harness Updates
// ...


void TestScatterLabels(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    
    // Robust Manual Construction
    std::array<float, 4> pos = {0.13f, 0.11f, 0.775f, 0.815f};
    auto ax = std::make_shared<matplot::axes_type>(f, pos);
    f->add_axes(ax, false, true);

    // Data
    std::vector<double> x = {15, 30, 45, 60, 75};
    std::vector<double> y = {10, 25, 15, 35, 20};
    
    // Scatter Plot (using 'o' marker style in plot is simpler for basic dots)
    auto l = ax->plot(x, y, "o");
    l->marker_face_color("blue");
    l->marker_size(10);

    // Labels
    std::vector<std::string> labels = {"Point A", "Point B", "Point C", "Point D", "Point E"};
    for(size_t i=0; i<x.size(); ++i) {
        // Offset label slightly
        ax->text(x[i] + 1, y[i] + 1, labels[i]);
    }

    ax->title("Scatter with Labels");
    ax->title_color({0, 0, 0, 1}); // Black
    ax->xlabel("X Value");
    ax->ylabel("Y Value");
    ax->box(true);
}

void TestHistogram(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    auto ax = f->add_axes();
    
    // Generate random-ish data (deterministic for testing)
    std::vector<double> data;
    for (int i = 0; i < 100; ++i) {
        data.push_back(std::sin(i * 0.3) * 5 + 10 + (i % 7) - 3);
    }
    
    ax->hist(data);
    ax->title("Histogram");
    ax->title_color({0, 0, 0, 1});
    ax->xlabel("Value");
    ax->ylabel("Frequency");
}

void TestStairs(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    auto ax = f->add_axes();
    
    std::vector<double> x = {0, 1, 2, 3, 4, 5, 6, 7};
    std::vector<double> y = {3, 4, 2, 5, 3, 6, 4, 7};
    
    ax->stairs(x, y);
    ax->title("Stairs (Step) Plot");
    ax->title_color({0, 0, 0, 1});
    ax->xlabel("X");
    ax->ylabel("Y");
}

void TestLogLog(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    auto ax = f->add_axes();
    
    std::vector<double> x = {1, 10, 100, 1000, 10000};
    std::vector<double> y = {1, 10, 100, 1000, 10000};
    
    ax->loglog(x, y);
    ax->title("Log-Log Plot");
    ax->title_color({0, 0, 0, 1});
    ax->xlabel("X (log)");
    ax->ylabel("Y (log)");
}

void TestErrorBar(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    auto ax = f->add_axes();
    
    std::vector<double> x = {1, 2, 3, 4, 5};
    std::vector<double> y = {2, 4, 3, 5, 4};
    std::vector<double> err = {0.5, 0.3, 0.4, 0.6, 0.3};
    
    ax->errorbar(x, y, err);
    ax->title("Error Bar Plot");
    ax->title_color({0, 0, 0, 1});
    ax->xlabel("X");
    ax->ylabel("Y with Error");
}

void TestPolarPlot(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    
    // Polar plot with theta and radius
    std::vector<double> theta, rho;
    for (double t = 0; t <= 2 * M_PI; t += 0.1) {
        theta.push_back(t);
        rho.push_back(1 + std::cos(t));
    }
    
    auto ax = f->add_axes(true);  // Polar axes
    ax->polarplot(theta, rho);
    ax->title("Polar (Cardioid)");
    ax->title_color({0, 0, 0, 1});
}

void TestPieChart(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    auto ax = f->add_axes();
    
    std::vector<double> values = {35, 25, 20, 15, 5};
    ax->pie(values);
    ax->title("Pie Chart");
    ax->title_color({0, 0, 0, 1});
}

void TestHeatmap(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    auto ax = f->add_axes();
    
    // Create 10x10 matrix
    std::vector<std::vector<double>> data(10, std::vector<double>(10));
    for (size_t i = 0; i < 10; ++i) {
        for (size_t j = 0; j < 10; ++j) {
            data[i][j] = std::sin(i * 0.5) * std::cos(j * 0.5);
        }
    }
    
    ax->heatmap(data);
    ax->title("Heatmap");
    ax->title_color({0, 0, 0, 1});
}

void TestContour(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    auto ax = f->add_axes();
    
    // Create grid data
    auto [X, Y] = matplot::meshgrid(
        matplot::linspace(-2, 2, 20),
        matplot::linspace(-2, 2, 20)
    );
    auto Z = matplot::transform(X, Y, [](double x, double y) {
        return std::exp(-(x*x + y*y));
    });
    
    ax->contour(X, Y, Z);
    ax->title("Contour Plot");
    ax->title_color({0, 0, 0, 1});
}

void TestBoxPlot(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    auto ax = f->add_axes();
    
    // Create sample data
    std::vector<std::vector<double>> data = {
        {1, 2, 3, 4, 5, 6, 7},
        {2, 3, 4, 5, 6, 7, 8},
        {3, 4, 5, 6, 7, 8, 10}
    };
    
    ax->boxplot(data);
    ax->title("Box Plot");
    ax->title_color({0, 0, 0, 1});
}

void TestQuiver(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    auto ax = f->add_axes();
    
    // Vector field
    std::vector<double> x = {0, 1, 2, 0, 1, 2, 0, 1, 2};
    std::vector<double> y = {0, 0, 0, 1, 1, 1, 2, 2, 2};
    std::vector<double> u = {1, 1, 1, 0, 0, 0, -1, -1, -1};
    std::vector<double> v = {0, 1, 0, 1, 0, -1, 0, -1, 0};
    
    ax->quiver(x, y, u, v);
    ax->title("Vector Field (Quiver)");
    ax->title_color({0, 0, 0, 1});
}

void TestMultiLine(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    auto ax = f->add_axes();
    
    std::vector<double> x = matplot::linspace(0, 2 * M_PI, 200);
    ax->plot(x, matplot::transform(x, [](double v){ return std::sin(v); }));
    ax->hold(true);
    ax->plot(x, matplot::transform(x, [](double v){ return std::cos(v); }));
    ax->plot(x, matplot::transform(x, [](double v){ return std::sin(2*v); }));
    ax->hold(false);
    
    ax->title("Multiple Lines");
    ax->title_color({0, 0, 0, 1});
    ax->xlabel("X");
    ax->ylabel("Y");
}

void TestScatter3D(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    auto ax = f->add_axes();
    
    // Random 3D points
    std::vector<double> x, y, z;
    for (int i = 0; i < 50; ++i) {
        double t = i * 0.2;
        x.push_back(std::cos(t) * (1 + 0.3 * std::sin(t * 3)));
        y.push_back(std::sin(t) * (1 + 0.3 * std::cos(t * 2)));
        z.push_back(t * 0.1);
    }
    
    ax->scatter3(x, y, z);
    ax->title("3D Scatter");
    ax->title_color({0, 0, 0, 1});
}

void TestStem3D(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
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
    ax->title_color({0, 0, 0, 1});
}

void TestSemiLogX(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    auto ax = f->add_axes();
    
    std::vector<double> x = {1, 10, 100, 1000, 10000};
    std::vector<double> y = {1, 2, 3, 4, 5};
    
    ax->semilogx(x, y);
    ax->title("Semi-Log X");
    ax->title_color({0, 0, 0, 1});
}

void TestSemiLogY(matplot::figure_handle f) {
    if (!f) return;
    f->children(std::vector<std::shared_ptr<matplot::axes_type>>{});
    auto ax = f->add_axes();
    
    std::vector<double> x = {1, 2, 3, 4, 5};
    std::vector<double> y = {1, 10, 100, 1000, 10000};
    
    ax->semilogy(x, y);
    ax->title("Semi-Log Y");
    ax->title_color({0, 0, 0, 1});
}

// --- Main Harness ---

int main() {
    try {
        Window window("Matplot++ WebGPU Gallery", 1024, 768);
        if (!window.Initialize()) return 1;

        auto renderer = std::make_shared<PrimitiveRenderer>(
            window.GetDevice(), window.GetFormat(),
            window.GetWidth(), window.GetHeight()
        );
        auto backend = std::make_shared<matplot::backend::WgpuBackend>(renderer);
        backend->width(window.GetWidth());
        backend->height(window.GetHeight());
        backend->set_render_size(window.GetWidth(), window.GetHeight());
        
        auto f = plt::figure(true);
        // std::cout << "Main: Created f=" << f->number() << std::endl;
        f->backend(backend);
        f->size(window.GetWidth(), window.GetHeight());

        // List of tests (Updated signature)
        std::vector<std::pair<std::string, std::function<void(matplot::figure_handle)>>> tests = {
            {"Line Plot", TestLinePlot},
            {"Scatter Plot", TestScatterPlot},
            {"Bar Chart", TestBarChart},
            {"Subplots", TestSubplot},
            {"3D Helix", Test3DLine},
            {"Stem Plot", TestStemPlot},
            {"Area Plot", TestAreaPlot},
            {"Scatter Labels", TestScatterLabels},
            {"Histogram", TestHistogram},
            {"Stairs Plot", TestStairs},
            {"Log-Log Plot", TestLogLog},
            {"Error Bars", TestErrorBar},
            {"Polar Plot", TestPolarPlot},
            {"Box Plot", TestBoxPlot},
            {"Vector Field", TestQuiver},
            {"Heatmap", TestHeatmap},
            {"Pie Chart", TestPieChart},
            {"Multi-Line", TestMultiLine},
            {"3D Scatter", TestScatter3D},
            {"3D Stem", TestStem3D},
            {"Semi-Log X", TestSemiLogX},
            {"Semi-Log Y", TestSemiLogY}
        };

        int current_test = 0;
        bool needs_update = true;

        // Helper to run current test
        auto run_test = [&]() {
            if (current_test < 0) current_test = tests.size() - 1;
            if (current_test >= tests.size()) current_test = 0;
            
            std::cout << "Switching to: " << tests[current_test].first << std::endl;
            // No need to switch global figure, just pass 'f'
            
            f->backend(backend); // Re-assert backend on F just in case
            f->size(window.GetWidth(), window.GetHeight());
            
            tests[current_test].second(f); // Pass F explicitly
        };

        // Input state
        bool left_pressed = false;
        bool right_pressed = false;

        window.Run([&](wgpu::RenderPassEncoder& pass) {
            // Input Handling (Simple Edge Detection)
            GLFWwindow* glWin = window.GetGLFWWindow();
            if (glfwGetKey(glWin, GLFW_KEY_RIGHT) == GLFW_PRESS) {
                if (!right_pressed) {
                    current_test++;
                    needs_update = true;
                    right_pressed = true;
                }
            } else {
                right_pressed = false;
            }

            if (glfwGetKey(glWin, GLFW_KEY_LEFT) == GLFW_PRESS) {
                if (!left_pressed) {
                    current_test--;
                    needs_update = true;
                    left_pressed = true;
                }
            } else {
                left_pressed = false;
            }

            // Detect Resize
            static uint32_t last_w = 0, last_h = 0;
            if (window.GetWidth() != last_w || window.GetHeight() != last_h) {
                last_w = window.GetWidth();
                last_h = window.GetHeight();
                if (last_w > 0 && last_h > 0) {
                    renderer->Resize(last_w, last_h);
                    backend->set_render_size(last_w, last_h);
                    backend->width(last_w);   // Sync logical size too for proper layout
                    backend->height(last_h);
                    f->size(last_w, last_h); // Sync figure size
                    needs_update = true;
                }
            }

            // Update Plot if needed
            if (needs_update) {
                run_test();
                needs_update = false;
            }

            // Render
            renderer->Clear();
            f->draw(); // Just to be sure backend has data (actually draw() pushes data to backend)
            // But wait, f->draw() *fills* the backend. We shouldn't call it every frame if data hasn't changed?
            // Actually f->draw() parses the plot hierarchy and calls backend methods. 
            // The backend accumulates primitives.
            // If we don't clear the backend, calling f->draw() again appends more primitives?
            // WgpuBackend::new_frame() clears primitives!
            // checks: does f->draw() call new_frame()? Yes usually.
            
            // Let's call f->draw() only on update. The backend retains data? 
            // My WgpuBackend::render_data() is what draws?
            // Re-reading WgpuBackend implementation...
            // It has `rects_`, `lines_`, etc. 
            // `new_frame()` clears them.
            // `render_data()` draws them using `renderer_`.
            
            // If I only call f->draw() once, the primitives are in WgpuBackend vectors.
            // Then loop calls `backend->render_data()`? 
            // My `WgpuBackend` does NOT have a render_data public method that executes WGPU commands. 
            // Wait, look at gpu_plot.cc again.
            // `f->draw()` populates backend.
            // Loop calls `f->draw()` every frame?
            // In `gpu_plot.cc`:
            // 77: f->draw();
            // 85: renderer->Render(pass);
            // So `f->draw()` re-generates all primitives every frame. This is inefficient but standard for immediate mode style.
            
            // So: call f->draw() every frame.
            f->draw(); 
            
            renderer->Prepare(window.GetQueue());
            renderer->Render(pass);
        });

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
