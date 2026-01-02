// Minimal test to verify rendering works
#include "minimal_renderer.h"
#include <iostream>
#include <fstream>

int main() {
  const int width = 800;
  const int height = 600;
  
  auto renderer = std::make_shared<mpl_wgpu::MinimalRenderer>(width, height);
  
  // Clear to white
  renderer->Clear({1.0f, 1.0f, 1.0f, 1.0f});
  
  // Draw a simple red diagonal line from (100,100) to (700,500)
  std::vector<mpl_wgpu::MinimalRenderer::Line> lines;
  lines.push_back({100.0f, 100.0f, 0.0f, 700.0f, 500.0f, 0.0f, 
                   1.0f, 0.0f, 0.0f, 1.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f});
  
  renderer->DrawLines(lines, width, height);
  
  // Draw a blue circle at center
  std::vector<mpl_wgpu::MinimalRenderer::Circle> circles;
  circles.push_back({400.0f, 300.0f, 0.0f, 50.0f,
                     0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f});
  
  renderer->DrawCircles(circles, width, height);
  
  // Save to file
  const uint8_t* pixels = renderer->GetPixels();
  std::ofstream ppm("test_output.ppm", std::ios::binary);
  ppm << "P6\n" << width << " " << height << "\n255\n";
  for (size_t i = 0; i < width * height; i++) {
    ppm.write(reinterpret_cast<const char*>(&pixels[i*4]), 3);
  }
  ppm.close();
  
  std::cout << "Test complete! Check test_output.ppm\n";
  std::cout << "Should show:\n";
  std::cout << "  - White background\n";
  std::cout << "  - Red diagonal line from top-left to bottom-right\n";
  std::cout << "  - Blue circle in center\n";
  
  return 0;
}
