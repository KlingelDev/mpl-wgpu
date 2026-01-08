# WgpuBackend Rendering Test Plan

This document outlines a comprehensive test plan to verify that all matplot++ 
example types render correctly with the WgpuBackend.

## Current Status

### ✅ Working (17 tests in `test_gallery`)

| # | Category | Type | Status | Notes |
|---|----------|------|--------|-------|
| 1 | Line Plot | `plot()` | ✅ Working | Basic line rendering |
| 2 | Data Distribution | `scatter()` | ✅ Working | Markers rendering |
| 3 | Discrete Data | `bar()` | ✅ Working | Filled rectangles |
| 4 | Appearance | Subplots | ✅ Working | Multiple axes |
| 5 | Surfaces | `plot3()` | ✅ Working | 3D helix line |
| 6 | Discrete Data | `stem()` | ✅ Working | Impulse + markers |
| 7 | Line Plot | `area()` | ✅ Working | Filled area |
| 8 | Annotations | Labels | ✅ Working | Text rendering |
| 9 | Data Distribution | `histogram()` | ✅ Working | Filled bars |
| 10 | Line Plot | `stairs()` | ✅ Working | Step lines |
| 11 | Line Plot | `loglog()` | ✅ Working | Log scale axes |
| 12 | Line Plot | `errorbar()` | ✅ Working | Error bars + caps |
| 13 | Polar Plots | `polarplot()` | ✅ Working | Polar coordinates |
| 14 | Data Distribution | `boxplot()` | ✅ Working | Whisker boxes |
| 15 | Vector Fields | `quiver()` | ✅ Working | Arrow vectors |
| 16 | Data Distribution | `heatmap()` | ✅ Working | Colored matrix |
| 17 | Line Plot | Multi-line | ✅ Working | Multiple overlaid |

---

### ❌ Not Yet Implemented

| Category | Type | Priority | Notes |
|----------|------|----------|-------|
| Data Distribution | `pie()` | Medium | Requires arc/circle drawing |
| Contour Plots | `contour()` | Medium | Requires contour extraction |
| Contour Plots | `contourf()` | Medium | Filled contours |
| Images | `imshow()` | Low | Image/texture rendering |
| Surfaces | `surf()` | Medium | 3D surface mesh |
| Surfaces | `mesh()` | Medium | 3D wireframe |
| Geography | `geoplot()` | Low | Map projections |
| Graphs | `graph()` | Low | Node/edge graphs |
| Annotations | `textarrow()` | Low | Arrow annotations |

---

## Test Categories (from matplotplusplus/examples/)
 
### 1. Line Plot (`line_plot/`)
- [x] `plot/` - Basic 2D line plots
- [x] `plot3/` - 3D line plots (helix tested)
- [x] `fplot/` - Function plots
- [ ] `fplot3/` - 3D function plots
- [x] `loglog/` - Log-log scale
- [ ] `semilogx/` - Semi-log (x-axis)
- [ ] `semilogy/` - Semi-log (y-axis)
- [x] `stairs/` - Step/stair plots
- [x] `area/` - Filled area plots
- [x] `errorbar/` - Error bar plots
- [ ] `fimplicit/` - Implicit function plots

### 2. Data Distribution (`data_distribution/`)
- [x] `scatter/` - 2D scatter plots
- [ ] `scatter3/` - 3D scatter plots
- [x] `histogram/` - Histograms
- [x] `boxplot/` - Box plots
- [x] `heatmap/` - Heatmaps
- [ ] `pie/` - Pie charts
- [ ] `binscatter/` - Binned scatter
- [ ] `parallelplot/` - Parallel coordinates
- [ ] `plotmatrix/` - Matrix of plots
- [ ] `wordcloud/` - Word clouds

### 3. Discrete Data (`discrete_data/`)
- [x] `bar/` - Bar charts
- [x] `stem/` - Stem plots
- [ ] `stem3/` - 3D stem plots
- [ ] `pareto/` - Pareto charts

### 4. Polar Plots (`polar_plots/`)
- [x] `polarplot/` - Polar line plots
- [ ] `polarscatter/` - Polar scatter
- [ ] `polarhistogram/` - Polar histogram
- [ ] `compass/` - Compass plots

### 5. Contour Plots (`contour_plots/`)
- [ ] `contour/` - Contour lines
- [ ] `contourf/` - Filled contours
- [ ] `fcontour/` - Function contours

### 6. Surfaces (`surfaces/`)
- [x] `plot3/` - 3D line (partial)
- [ ] `surf/` - Surface plots
- [ ] `mesh/` - Mesh plots
- [ ] `ribbon/` - Ribbon plots
- [ ] `waterfall/` - Waterfall plots

### 7. Vector Fields (`vector_fields/`)
- [x] `quiver/` - 2D vector arrows
- [ ] `quiver3/` - 3D vector arrows
- [ ] `feather/` - Feather plots

### 8. Images (`images/`)
- [ ] `imshow/` - Display images
- [ ] `imagesc/` - Scaled images

### 9. Annotations (`annotations/`)
- [x] `text/` - Text labels
- [ ] `textarrow/` - Text with arrows
- [ ] `rectangle/` - Rectangle annotations
- [ ] `arrow/` - Arrow annotations
- [ ] `line/` - Line annotations

### 10. Appearance (`appearance/`)
- [x] Titles, labels, colors
- [ ] Legends
- [ ] Colormaps
- [ ] Axis limits

### 11. Geography (`geography/`)
- [ ] `geoplot/` - Geographic plots
- [ ] `worldmap/` - World maps

### 12. Graphs (`graphs/`)
- [ ] `graph/` - Node/edge graphs
- [ ] `digraph/` - Directed graphs

### 13. Exporting (`exporting/`)
- [ ] PNG export
- [ ] SVG export
- [ ] PDF export

---

## Reference Image Strategy

### Option 1: Use Gnuplot Output (Recommended)
1. Run each example with the default Gnuplot backend
2. Export to PNG as reference image
3. Store in `docs/reference_images/`

### Option 2: Use matplotplusplus Gallery
The matplotplusplus documentation includes rendered examples:
- https://alandefreitas.github.io/matplotplusplus/

---

## Testing Workflow

### For Each Plot Type:

1. **Create Test Function**
   ```cpp
   void Test<PlotType>(matplot::figure_handle f) {
       // Setup plot data
       // Call matplot function
       // Set title for identification
   }
   ```

2. **Register in Gallery**
   ```cpp
   tests.push_back({"<Name>", Test<PlotType>});
   ```

3. **Visual Verification**
   - Run `test_gallery`
   - Cycle to the new test
   - Compare with reference image

4. **If Not Rendering:**
   - Check if `run_draw_commands()` exists in the axes_object class
   - Implement if missing
   - Add declaration to header file

---

## Priority Order for Implementation

1. **High Priority** (commonly used):
   - `semilogx/semilogy` - Log scale variants
   - `scatter3` - 3D scatter
   - `contour/contourf` - Contour plots
   - `pie` - Pie charts

2. **Medium Priority**:
   - `surf/mesh` - 3D surfaces
   - `stem3` - 3D stem
   - `legends` - Legend rendering
   - `colormaps` - Proper colormap support

3. **Low Priority**:
   - `geography` - Map projections
   - `graphs` - Node/edge visualization
   - `wordcloud` - Text clouds
