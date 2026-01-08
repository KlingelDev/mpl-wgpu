# mpl-wgpu: wgpu Rendering Backend for matplotplusplus

## Architecture

`mpl-wgpu` provides **GPU-accelerated rendering** for [matplotplusplus](https://github.com/alandefreitas/matplotplusplus), bringing the full power of this mature C++ plotting library to Rust with hardware acceleration via wgpu.

**Components:**
1. **wgpu Backend (C++)** - Implements matplotplusplus `backend_interface` using wgpu
2. **C API** - C-compatible interface for FFI
3. **Rust Bindings** - Safe, idiomatic Rust wrapper

**Benefits:**
- 🚀 GPU-accelerated rendering via wgpu
- 🦀 Use full matplotplusplus API from Rust
- 🎨 All matplotplusplus plot types available immediately
- 🔧 Leverages mature, well-tested plotting library
- 💻 Cross-platform (Windows, Linux, macOS, WASM)

## Implementation Roadmap

## Implementation Roadmap

### ✅ Phase 1: Core Backend Implementation (v0.1.0) - COMPLETE
**Goal:** Functional C++ backend rendering to pixel buffer

**Deliverables:**
- [x] C++ backend class (`WgpuBackend`) inheriting from `matplot::backend::backend_interface`
- [x] Implementation of core primitives:
  - [x] `draw_rectangle()` - Backgrounds and bars
  - [x] `draw_path()` - Lines and curves
  - [x] `draw_markers()` - Scatter plot points (circles)
  - [x] `draw_triangle()` - Filled areas and 3D surfaces
  - [x] `draw_text()` - Basic text placement
- [x] WebGPU Integration:
  - [x] Custom C++ wrapper for `wgpu-native`
  - [x] Shader compilation and pipeline state management
  - [x] Vertex and index buffer management
- [x] Coordinate System:
  - [x] Correct mapping from matplot++ (0-100) to pixel coordinates
- [x] Build System:
  - [x] CMake integration with `FetchContent` for dependencies
  - [x] No external dependencies required (auto-downloads wgpu-native)

### Phase 2: Native Surface & Performance (v0.2.0)
**Goal:** Direct rendering to window surface and optimization

**Deliverables:**
- [ ] **Surface Integration:**
  - [ ] Remove OpenGL dependency (currently used for display)
  - [ ] Implement direct `wgpu::Surface` configuration
  - [ ] Support SwapChain resizing
- [ ] **Text Rendering:**
  - [ ] Implement proper font atlas system
  - [ ] Support high-quality text rendering (SDF or high-res texture)
- [ ] **Optimization:**
  - [ ] Instanced rendering for markers (currently 1 draw call per marker group)
  - [ ] Uniform buffer optimization

### Phase 3: Rust Bindings (v0.3.0) ✅ COMPLETE
**Goal:** Safe, idiomatic Rust API

**Deliverables:**
- [x] **FFI Layer:**
  - [x] Expose C API for backend creation/destruction
  - [x] Expose render functions
- [x] **Rust Wrapper:**
  - [x] `mpl-wgpu` crate structure
  - [x] Safe wrappers for `Figure`, `Axes`
  - [x] Integration with `winit` for window management

### Phase 4: Interactivity & Advanced Features (v0.4.0)
**Goal:** Full interactive plotting support

**Deliverables:**
- [ ] Mouse and keyboard event handling
- [ ] Zoom and pan support
- [ ] Real-time data updates (streaming plots)
- [ ] 3D camera controls
- [ ] Export to image formats (PNG/buffer capture)

---

## Current Status

**✅ Completed:**
- Fully functional C++ backend (`WgpuBackend`)
- WebGPU rendering engine (`PrimitiveRenderer`) using `wgpu-native`
- **Idiomatic Rust API** (`Figure`, `Axes`) accessing Backend
- **Implemented Plot Types** in Rust:
  - `plot` (Line plots)
  - `scatter` (Markers)
  - `bar` (Bar charts)
  - `hist` (Histograms)
  - `surf` (3D Surfaces/Heatmaps)
- Verified rendering of basic plot types:
  - Line plots (axes, grids, data)
  - Scatter plots (markers)
  - Filled areas (rectangles)
- Example application (`simple_plot`, `plotting_showcase`) rendering to window

**🚧 Immediate Focus:**
- Transitioning from software-rasterized pixel buffer to direct GPU presentation
- Creating Rust FFI bindings to make this usable from Rust projects

**📋 Known Issues:**
- Text rendering is basic
- Current display uses OpenGL for presentation (rendering is WGPU-based but blitted via OpenGL)

---

## Why This Approach?

**Hybrid Architecture:**
- **C++ Core:** Leverages existing high-performance C++ backend code
- **Rust Shell:** Provides safe, ergonomic Rust API
- **WebGPU:** Modern, cross-platform low-level graphics API

**Benefits:**
- **Performance:** Hardware acceleration for large datasets
- **Portability:** Runs on Vulkan, Metal, DX12, and WebGPU (browser)
- **Feature Parity:** Instant access to 100+ plot types from `matplot++` without reimplementation

---

## Contributing

Priority areas:
1. **Rust Integration:** Helping build the safe Rust bindings
2. **Text Rendering:** Improving font quality and performance
3. **Surface Support:** Implementing platform-specific surface creation for Windows/Linux/macOS


### 1.1 Line Plots Enhancement
- [ ] Stair plots
- [ ] Area plots (filled under curve)
- [ ] Error bars (vertical and horizontal)
- [ ] Stem plots
- [ ] Impulse plots
- [ ] Log-scale plots (log, semilogx, semilogy, loglog)

### 1.2 Data Distribution
- [x] Histograms
- [ ] Box plots
- [ ] Violin plots
- [ ] Stem-and-leaf plots
- [ ] Pie charts
- [ ] Pareto charts
- [ ] Word clouds
- [x] Heatmaps

### 1.3 Discrete Data
- [ ] Grouped bar charts
- [ ] Stacked bar charts
- [ ] Horizontal bar charts
- [ ] Binned scatter plots

## Phase 2: Advanced Visualizations (v0.2.0)

### 2.1 Contour Plots
- [ ] Basic contour lines
- [ ] Filled contours (contourf)
- [ ] Function contours (fcontour)
- [ ] Contour labels
- [ ] Custom contour levels

### 2.2 Vector Fields
- [ ] Quiver plots (2D vector fields)
- [ ] 3D quiver plots
- [ ] Feather plots
- [ ] Compass plots

### 2.3 Polar Plots
- [ ] Polar line plots (polarplot)
- [ ] Polar scatter (polarscatter)
- [ ] Polar histograms (polarhistogram)
- [ ] Compass plots
- [ ] Rose diagrams (wind rose)

### 2.4 Geography
- [ ] Geographic plots (geoplot)
- [ ] Geographic scatter (geoscatter)
- [ ] Geographic bubble charts (geobubble)
- [ ] Map projections
- [ ] Basemaps

## Phase 3: Annotations & Appearance (v0.3.0)

### 3.1 Annotations
- [ ] Text annotations
- [ ] Arrow annotations
- [ ] Text with arrow (textarrow)
- [ ] Text boxes
- [ ] Rectangles
- [ ] Ellipses
- [ ] Filled regions
- [ ] Custom shapes

### 3.2 Appearance Enhancements
- [ ] Colormaps (jet, hsv, hot, cool, spring, summer, autumn, winter, gray, bone, copper, pink, parula)
- [ ] Multiple subplots (subplot)
- [ ] Tiled layouts (tiledlayout)
- [ ] Figure sizing and DPI
- [ ] Background colors
- [ ] Transparency/alpha support
- [ ] Font customization
- [ ] Legend positioning and styling
- [ ] Axis ticks customization
- [ ] Multiple y-axes (yyaxis)

## Phase 4: Graphs & Networks (v0.4.0)

- [ ] Graph plots (directed/undirected)
- [ ] Tree diagrams
- [ ] Network visualizations
- [ ] Node and edge styling
- [ ] Force-directed layouts

## Phase 5: Images & Advanced Features (v0.5.0)

### 5.1 Image Handling
- [ ] Image display (imshow)
- [ ] Image with transparency
- [ ] Image scaling and interpolation
- [ ] Pixel grids

### 5.2 Statistical Plots
- [ ] Regression lines
- [ ] Confidence intervals
- [ ] Distribution fitting
- [ ] Probability plots

### 5.3 Animation Support
- [ ] Real-time plot updates (already partially supported)
- [ ] Frame-by-frame animation
- [ ] Smooth transitions
- [ ] Animation export

## Phase 6: Export & Integration (v0.6.0)

### 6.1 Export Formats
- [ ] PNG export
- [ ] SVG export
- [ ] PDF export
- [ ] EPS export
- [ ] LaTeX/TikZ export

### 6.2 Integration Features
- [ ] Interactive callbacks
- [ ] Mouse event handling
- [ ] Zoom and pan controls
- [ ] Data cursor/tooltips
- [ ] Plot templates

## Technical Improvements (Ongoing)

### Performance
- [ ] Optimize large dataset rendering (>1M points)
- [ ] Level-of-detail (LOD) for dense plots
- [ ] GPU instancing for markers
- [ ] Compute shader for histogram binning
- [ ] Spatial indexing for pick/selection

### API Design
- [ ] Builder pattern for all plot types
- [ ] Chainable method calls
- [ ] Macro support for quick plotting
- [ ] Async/await support for long operations

### Documentation
- [ ] Example gallery (mirroring matplot++)
- [ ] API documentation for all modules
- [ ] Tutorials for common use cases
- [ ] Migration guide from matplotlib/matplot++

## Priority Matrix

**High Priority (Essential for Scientific Computing):**
1. Error bars
2. Box plots
3. Heatmaps
4. Log-scale plots
5. Contour plots
6. Colormaps
7. Subplots/layouts
8. Legend positioning
9. Export formats (PNG, SVG)

**Medium Priority (Commonly Used):**
1. Pie charts
2. Stacked/grouped bars
3. Polar plots
4. Filled areas
5. Regression lines
6. Animation
7. Interactive features

**Low Priority (Specialized):**
1. Geography plots
2. Word clouds
3. Graphs/networks
4. Pareto charts
5. Feather plots

## Compatibility Goals

- **API Compatibility**: 90% compatible with matplot++ API patterns
- **Feature Coverage**: 100% of core features, 80% of specialized features
- **Performance**: 10x faster than CPU rendering for >10k+ points
- **Cross-platform**: Windows, Linux, macOS, Web (via wasm)

## Contributing

Contributions welcome! Priority areas:
1. Implementing missing plot types (see Phase 1-2)
2. Adding examples from matplot++ gallery
3. Performance optimizations
4. Documentation and tutorials
5. Export format support
