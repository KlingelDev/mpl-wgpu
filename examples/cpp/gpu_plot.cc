#include "window.h"
#include "../../src/backend/primitive_renderer.h"
#include <matplot/matplot.h>
#include <iostream>

using namespace mpl_wgpu;

int main() {
    try {
        std::cout << "Creating window..." << std::endl;
        Window window("GPU Plot Example", 800, 600);
        if (!window.Initialize()) {
            return 1;
        }
        std::cout << "Window initialized!" << std::endl;

        // Initialize Renderer
        auto renderer = std::make_shared<PrimitiveRenderer>(
            window.GetDevice(),
            window.GetFormat(),
            window.GetWidth(),
            window.GetHeight()
        );
        std::cout << "Renderer created!" << std::endl;

        // Create WgpuBackend
        auto backend = std::make_shared<matplot::backend::WgpuBackend>(renderer);
        // Important: Set backend size so matplot knows mapping range
        backend->width(window.GetWidth());
        backend->height(window.GetHeight());
        
        // Create Figure and attaching backend
        auto f = matplot::figure(true);
        f->backend(backend);
        
        // Explicitly set render size to match window init
        renderer->Resize(window.GetWidth(), window.GetHeight());
        backend->set_render_size(window.GetWidth(), window.GetHeight());

        // Setup simple plot on current axes
        std::vector<double> x = {1, 2, 3, 4, 5};
        std::vector<double> y = {1, 4, 9, 16, 25};
        auto l = f->current_axes()->plot(x, y);
        l->line_width(3.0f); 
        f->current_axes()->title("GPU Plot Test");
        f->current_axes()->title_color({0.0f, 0.0f, 0.0f, 1.0f}); // Black title
        
        f->current_axes()->xlabel("X Axis");
        f->current_axes()->x_axis().label_color({0.0f, 0.0f, 0.0f, 1.0f}); 
        f->current_axes()->y_axis().label_color({0.0f, 0.0f, 0.0f, 1.0f});
        
        // Force initial draw to populate backend
        f->draw();

        // Render Loop
        int frame = 0;
        uint32_t current_width = window.GetWidth();
        uint32_t current_height = window.GetHeight();

        window.Run([&](wgpu::RenderPassEncoder& pass) {
            // Check for resize
            if (window.GetWidth() != current_width || window.GetHeight() != current_height) {
                current_width = window.GetWidth();
                current_height = window.GetHeight();
                if (current_width > 0 && current_height > 0) {
                    renderer->Resize(current_width, current_height);
                    backend->set_render_size(current_width, current_height);
                    // Do NOT update logical size (backend->width/height or f->width/height)
                    // to preserve aspect ratio via backend centering logic.
                    f->draw(); // Redraw
                }
            }

            renderer->Clear();
           
            // Render the data accumulated in the backend
            f->draw();
            
            // Specific backend flush if needed? 
            
            // Specific backend flush if needed? 
            // backend->render_data(); // depends on impl
            
            renderer->Prepare(window.GetQueue());
            renderer->Render(pass);
            
            if (frame++ % 60 == 0) {
                // std::cout << "Frame " << frame << std::endl;
            }
        });

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
