# mpl-wgpu

Hardware-accelerated backend for matplotlib++ using [wgpu](https://wgpu.rs/).

## Features

- 🚀 **GPU-Accelerated**: Fast rendering using wgpu for all plot types
- 📊 **Rich Plot Types**: Line plots, scatter plots, histograms, bar charts, 3D surfaces
- 🎨 **Customizable**: Full control over colors, line styles, markers, and labels
- 📐 **Flexible Axes**: Automatic or manual scaling, grid lines, custom ranges
- 🖼️ **Multiple Backends**: Render to windows, textures, or offscreen buffers
- ⚡ **Real-time Updates**: Designed for dynamic, updating visualizations

## Installation

Add this to your `Cargo.toml`:

```toml
[dependencies]
mpl-wgpu = "0.1"
wgpu = "0.20"
glam = "0.25"
```

## Quick Start

```rust
use mpl_wgpu::{PlotBackend, PrimitiveRenderer, TextRenderer};
use glam::Vec4;

// Create plot backend
let mut plot = PlotBackend::new(800, 600);

// Configure axes
plot.axis_mut().title = "My Plot".into();
plot.axis_mut().x_label = "X Axis".into();
plot.axis_mut().y_label = "Y Axis".into();

// Add data
let x: Vec<f64> = (0..100).map(|i| i as f64 * 0.1).collect();
let y: Vec<f64> = x.iter().map(|&x| x.sin()).collect();
let color = Vec4::new(0.0, 0.5, 1.0, 1.0);

plot.plot(x, y, color);
plot.auto_scale();

// Render (requires wgpu setup)
// plot.render(&mut prim_renderer, &mut text_renderer, None);
```

## Plot Types

### Line Plots
```rust
plot.plot(x_data, y_data, color);
plot.plot_styled(x_data, y_data, color, line_width, line_style);
```

### Scatter Plots
```rust
plot.scatter(x_data, y_data, color, marker_size);
plot.scatter_styled(x_data, y_data, color, size, marker_style);
```

### Histograms
```rust
plot.histogram(&data, num_bins, color);
```

### Bar Charts
```rust
plot.bar(values, color);
```

### 3D Surfaces
```rust
plot.surface(x_grid, y_grid, z_values, color);
plot.surface_with_colormap(x_grid, y_grid, z_values);
```

## Architecture

`mpl-wgpu` is built on three core components:

- **`PlotBackend`**: High-level plotting API (matplotlib-style)
- **`PrimitiveRenderer`**: Low-level GPU primitive rendering (lines, circles, rectangles)
- **`TextRenderer`**: GPU-accelerated text rendering

All rendering is done via compute and render pipelines on the GPU for maximum performance.

## Examples

See the [examples directory](examples/) for complete examples including:
- Basic line plots
- Real-time data visualization
- Multi-window applications
- Histogram analysis

## Integration

`mpl-wgpu` is designed to integrate with existing wgpu applications:

```rust
// In your render loop
plot_backend.render(&mut prim_renderer, &mut text_renderer, None);
prim_renderer.prepare(&device, &queue);
text_renderer.prepare(&device, &queue);

// In your render pass
prim_renderer.render(&mut render_pass);
text_renderer.render(&mut render_pass);
```

## License

MIT License - see [LICENSE](LICENSE) for details.