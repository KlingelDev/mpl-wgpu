# mpl-wgpu Roadmap to Feature Parity with Matplot++

This roadmap outlines the path to achieving complete feature parity with the [matplotplusplus](https://github.com/alandefreitas/matplotplusplus) library.

## Current Status (v0.0.1)

**Implemented:**
- ✅ Basic 2D line plots
- ✅ 2D scatter plots  
- ✅ Basic 3D line plots and scatter
- ✅ 3D surfaces with mesh and colormap
- ✅ Histograms
- ✅ Bar charts
- ✅ Basic axis configuration (labels, titles, limits)
- ✅ Auto-scaling
- ✅ Grid lines
- ✅ Line styles (solid, dashed, dotted)
- ✅ Marker styles (circle, square, diamond, plus, cross, point)

## Phase 1: Core Plot Types (v0.1.0)

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

## Release Schedule

- **v0.1.0** (Q1 2026): Core plot types
- **v0.2.0** (Q2 2026): Advanced visualizations
- **v0.3.0** (Q3 2026): Annotations & appearance
- **v0.4.0** (Q4 2026): Graphs & networks
- **v0.5.0** (Q1 2027): Images & statistical
- **v0.6.0** (Q2 2027): Export & integration
- **v1.0.0** (Q3 2027): Feature-complete, production-ready

## Contributing

Contributions welcome! Priority areas:
1. Implementing missing plot types (see Phase 1-2)
2. Adding examples from matplot++ gallery
3. Performance optimizations
4. Documentation and tutorials
5. Export format support
