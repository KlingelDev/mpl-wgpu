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

### Phase 1: Minimal wgpu Backend (v0.1.0)

**Goal:** Basic functional backend in C++

**Deliverables:**
- [ ] C++ wgpu backend class inheriting from `backend_interface`
- [ ] Implement core rendering methods:
  - [ ] `draw_background()` - Clear/fill background
  - [ ] `draw_rectangle()` - Filled rectangles  
  - [ ] `draw_path()` - Line rendering
  - [ ] `new_frame()` / `render_data()` - Frame lifecycle
- [ ] Integration with existing `PrimitiveRenderer`
- [ ] Build system (CMake integration)
- [ ] Simple test: render a basic 2D line plot

**What we keep from current code:**
- ✅ `PrimitiveRenderer` - Reuse for backend implementation
- ✅ `TextRenderer` - Reuse for text drawing
- ✅ Core wgpu infrastructure

**What we remove:**
- ❌ `PlotBackend` Rust struct (matplot++ handles this)
- ❌ Rust plot implementations (error bars, area plots, etc.)

---

### Phase 2: Complete Backend Implementation (v0.2.0)

**Goal:** Full vertex-based backend support

**Deliverables:**
- [ ] Implement remaining drawing methods:
  - [ ] `draw_markers()` - Point/marker rendering
  - [ ] `draw_text()` - Text rendering via `TextRenderer`
  - [ ] `draw_triangle()` - Triangle primitives
  - [ ] `draw_image()` - Image/texture display
- [ ] Coordinate transformation system
- [ ] Color management
- [ ] Z-buffer/depth handling for 3D plots
- [ ] Test suite covering all primitives

---

### Phase 3: C API Layer (v0.3.0)

**Goal:** Expose backend via C API for FFI

**Deliverables:**
- [ ] Design C API surface
- [ ] Export functions:
  ```c
  mpl_backend* mpl_wgpu_create(uint32_t w, uint32_t h);
  void mpl_wgpu_destroy(mpl_backend*);
  void mpl_wgpu_render(mpl_backend*, mpl_figure*);
  uint8_t* mpl_wgpu_get_pixels(mpl_backend*);
  ```
- [ ] Error handling across FFI boundary
- [ ] Memory management strategy
- [ ] C API documentation

---

### Phase 4: Rust FFI Bindings (v0.4.0)

**Goal:** Safe Rust wrapper around matplot++ with wgpu backend

**Deliverables:**
- [ ] Generate `bindgen` bindings
- [ ] Create safe Rust wrappers:
  ```rust
  pub struct Figure { /* ... */ }
  pub struct Axes { /* ... */ }
  pub struct WgpuBackend { /* ... */ }
  ```
- [ ] Implement plot type methods:
  - [ ] `plot()`, `plot3()`
  - [ ] `scatter()`, `scatter3()`
  - [ ] `bar()`, `histogram()`
  - [ ] `surf()`, `mesh()`
  - [ ] All matplot++ plot types
- [ ] Idiomatic Rust API
- [ ] Comprehensive examples
- [ ] Documentation and tutorials

---

### Phase 5: Integration & Polish (v0.5.0)

**Goal:** Production-ready release

**Deliverables:**
- [ ] Port matplot++ example gallery to Rust
- [ ] Performance benchmarks
- [ ] Memory leak analysis (Valgrind/AddressSanitizer)
- [ ] Cross-platform testing (Linux, Windows, macOS)
- [ ] WASM support
- [ ] CI/CD pipeline
- [ ] Published crate on crates.io

---

### Phase 6: Advanced Features (v1.0.0)

**Goal:** Enhanced integration and performance

**Deliverables:**
- [ ] Interactive plots (mouse/keyboard)
- [ ] Animation support
- [ ] Real-time data streaming
- [ ] Multiple figure windows
- [ ] Custom colormaps
- [ ] Theme system
- [ ] Export formats (PNG, SVG, PDF)

---

## Current Status

**✅ Completed:**
- Research of matplotplusplus backend interface
- Analysis of existing backends (gnuplot, OpenGL)
- Identified reusable components (`PrimitiveRenderer`, `TextRenderer`)

**🚧 In Progress:**
- Setting up build system

**📋 Next Steps:**
1. Add matplotplusplus as git submodule
2. Create `wgpu_backend.h/cpp` stub
3. Implement minimal backend
4. Test with simple plot

---

## Why This Approach?

**vs. Reimplementing Everything in Rust:**
- ✅ Get all matplot++ features immediately (100+ plot types)
- ✅ Leverage mature, well-tested codebase
- ✅ Focus on GPU rendering optimization
- ✅ Smaller maintenance burden

**Trade-offs:**
- ⚠️ C++ dependency (but isolated in backend)
- ⚠️ FFI overhead (minimal for rendering)
- ✅ Best-in-class plotting + best-in-class GPU rendering

---

## Contributing

Priority areas:
1. Implementing backend primitive methods
2. Testing across platforms
3. Performance optimization
4. Documentation and examples
5. WASM compatibility


### 1.1 Line Plots Enhancement
- [ ] Stair plots
- [ ] Area plots (filled under curve)
- [ ] Error bars (vertical and horizontal)
- [ ] Stem plots
- [ ] Impulse plots
- [ ] Log-scale plots (log, semilogx, semilogy, loglog)

### 1.2 Data Distribution
- [ ] Box plots
- [ ] Violin plots
- [ ] Stem-and-leaf plots
- [ ] Pie charts
- [ ] Pareto charts
- [ ] Word clouds
- [ ] Heatmaps

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