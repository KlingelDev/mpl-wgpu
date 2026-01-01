// Copyright (c) 2025 Karl Ruskowski
// SPDX-License-Identifier: MIT
//
// Plotting backend for matplotlib-style plotting using wgpu primitives.

use crate::primitives::PrimitiveRenderer;
use glam::{Vec2, Vec4};

/// Configuration for plot axes including limits, labels, and display options.
///
/// # Examples
///
/// ```
/// use mpl_wgpu::AxisConfig;
///
/// let mut axis = AxisConfig::default();
/// axis.title = "My Plot".into();
/// axis.x_label = "Time (s)".into();
/// axis.y_label = "Value".into();
/// axis.x_min = 0.0;
/// axis.x_max = 10.0;
/// ```
#[derive(Debug, Clone)]
pub struct AxisConfig {
  pub x_min: f64,
  pub x_max: f64,
  pub y_min: f64,
  pub y_max: f64,
  pub z_min: f64,
  pub z_max: f64,
  pub title: String,
  pub x_label: String,
  pub y_label: String,
  pub z_label: String,
  pub show_grid: bool,
  pub x_scale: AxisScale,
  pub y_scale: AxisScale,
}

impl Default for AxisConfig {
  fn default() -> Self {
    Self {
      x_min: 0.0,
      x_max: 1.0,
      y_min: 0.0,
      y_max: 1.0,
      z_min: 0.0,
      z_max: 1.0,
      title: String::new(),
      x_label: String::new(),
      y_label: String::new(),
      z_label: String::new(),
      show_grid: true,
      x_scale: AxisScale::Linear,
      y_scale: AxisScale::Linear,
    }
  }
}

/// Axis scaling type for transforming data coordinates.
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum AxisScale {
    /// Linear scaling (no transformation)
    Linear,
    /// Base-10 logarithmic scaling
    Log10,
    /// Natural logarithm scaling  
    Ln,
}

impl Default for AxisScale {
    fn default() -> Self {
        Self::Linear
    }
}

/// Line style options for rendering paths and series.
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum LineStyle {
  /// Solid continuous line
  Solid,
  /// Dashed line pattern
  Dashed,
  /// Dotted line pattern
  Dotted,
}

/// Marker style options for scatter plots and point rendering.
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum MarkerStyle {
  /// Circular marker
  Circle,
  /// Square marker
  Square,
  /// Diamond-shaped marker
  Diamond,
  /// Plus sign (+)
  Plus,
  /// Cross/X marker
  Cross,
  /// Small point
  Point,
}

/// Error bar data for representing uncertainty in measurements.
#[derive(Debug, Clone)]
pub enum ErrorData {
    /// No error bars
    None,
    /// Symmetric error bars (same magnitude above and below)
    Symmetric { values: Vec<f64> },
    /// Asymmetric error bars (different magnitudes above and below)
    Asymmetric { low: Vec<f64>, high: Vec<f64> },
}

/// Configuration for error bar rendering.
#[derive(Debug, Clone, Copy)]
pub struct ErrorBarStyle {
    /// Width of error bar line
    pub line_width: f32,
    /// Width of cap at error bar ends (0.0 = no cap)
    pub cap_width: f32,
    /// Whether to draw error bars
    pub enabled: bool,
}

impl Default for ErrorBarStyle {
    fn default() -> Self {
        Self {
            line_width: 1.5,
            cap_width: 8.0,
            enabled: true,
        }
    }
}

/// A 3D surface mesh for surface plots.
#[derive(Debug, Clone)]
pub struct Surface {
    pub x: Vec<f64>,
    pub y: Vec<f64>,
    /// Z values in flattened nx * ny format
    pub z: Vec<f64>,
    pub color: Vec4,
    pub use_colormap: bool,
}

/// An area plot (filled region under a curve).
#[derive(Debug, Clone)]
pub struct AreaSeries {
    pub x: Vec<f64>,
    pub y: Vec<f64>,
    pub color: Vec4,
    /// Y-value for the baseline (default 0.0)
    pub baseline: f64,
    /// Transparency/alpha for the fill
    pub alpha: f32,
}

impl Default for AreaSeries {
    fn default() -> Self {
        Self {
            x: Vec::new(),
            y: Vec::new(),
            color: Vec4::new(0.0, 0.5, 1.0, 0.3),
            baseline: 0.0,
            alpha: 0.3,
        }
    }
}

/// A data series containing x, y, (optional z) coordinates and styling.
///
/// Can represent line plots, scatter plots, or 3D curves depending on
/// the presence of z data and marker styles.
#[derive(Debug, Clone)]
pub struct Series {
    pub x: Vec<f64>,
    pub y: Vec<f64>,
    /// Optional Z coordinates for 3D plots
    pub z: Vec<f64>,
    pub color: Vec4,
    pub line_width: f32,
    pub line_style: LineStyle,
    pub marker_style: Option<MarkerStyle>,
    pub marker_size: f32,
    /// X-axis error data
    pub x_error: ErrorData,
    /// Y-axis error data
    pub y_error: ErrorData,
    /// Error bar styling
    pub error_style: ErrorBarStyle,
}

impl Default for Series {
    fn default() -> Self {
        Self {
            x: Vec::new(),
            y: Vec::new(),
            z: Vec::new(),
            color: Vec4::new(0.0, 0.5, 1.0, 1.0),
            line_width: 3.0,
            line_style: LineStyle::Solid,
            marker_style: None,
            marker_size: 10.0,
            x_error: ErrorData::None,
            y_error: ErrorData::None,
            error_style: ErrorBarStyle::default(),
        }
    }
}

/// High-level plotting backend with matplotlib-style API.
///
/// Provides methods for creating line plots, scatter plots, histograms,
/// bar charts, and 3D surfaces. All rendering is GPU-accelerated via wgpu.
///
/// # Examples
///
/// ```rust,no_run
/// use mpl_wgpu::{PlotBackend, PrimitiveRenderer, TextRenderer};
/// use glam::Vec4;
///
/// let mut plot = PlotBackend::new(800, 600);
///
/// // Configure
/// plot.axis_mut().title = "Sine Wave".into();
///
/// // Add data
/// let x: Vec<f64> = (0..100).map(|i| i as f64 * 0.1).collect();
/// let y: Vec<f64> = x.iter().map(|&x| x.sin()).collect();
/// plot.plot(x, y, Vec4::new(1.0, 0.0, 0.0, 1.0));
/// plot.auto_scale();
///
/// // Render with wgpu renderers
/// // plot.render(&mut prim_renderer, &mut text_renderer, None);
/// ```
pub struct PlotBackend {
    width: u32,
    height: u32,
    margin_left: f32,
    margin_right: f32,
    margin_top: f32,
    margin_bottom: f32,
    axis: AxisConfig,
    series: Vec<Series>,
    bars: Vec<(Vec<f64>, Vec4)>,
    histogram_bins: Vec<(f64, f64, f64, Vec4)>,
    surfaces: Vec<Surface>,
    areas: Vec<AreaSeries>,
    background_color: Vec4,
    scale_factor: f32,
}

impl PlotBackend {
    /// Creates a new plot backend with the specified dimensions.
    ///
    /// # Arguments
    ///
    /// * `width` - Width in pixels
    /// * `height` - Height in pixels
    pub fn new(width: u32, height: u32) -> Self {
        Self {
            width,
            height,
            margin_left: 80.0,
            margin_right: 40.0,
            margin_top: 50.0,
            margin_bottom: 60.0,
            axis: AxisConfig::default(),
            series: Vec::new(),
            bars: Vec::new(),
            histogram_bins: Vec::new(),
            surfaces: Vec::new(),
            areas: Vec::new(),
            background_color: Vec4::new(0.93, 0.93, 0.93, 1.0),
            scale_factor: 1.0,
        }
    }

    /// Resizes the plot to new dimensions.
    pub fn resize(&mut self, width: u32, height: u32) {
        self.width = width;
        self.height = height;
    }

    /// Adds a pre-configured series to the plot.
    pub fn add_series(&mut self, series: Series) {
        self.series.push(series);
    }

    /// Sets the DPI scale factor for high-resolution displays.
    pub fn set_scale_factor(&mut self, factor: f32) {
        self.scale_factor = factor;
        self.margin_left = 80.0 * factor;
        self.margin_right = 40.0 * factor;
        self.margin_top = 50.0 * factor;
        self.margin_bottom = 60.0 * factor;
    }

    /// Clears all plot data including series, bars, histograms, and surfaces.
    pub fn clear(&mut self) {
        self.series.clear();
        self.bars.clear();
        self.histogram_bins.clear();
        self.surfaces.clear();
        self.areas.clear();
        self.axis = AxisConfig::default();
    }

    /// Sets the axis configuration.
    pub fn set_axis(&mut self, axis: AxisConfig) {
        self.axis = axis;
    }

    /// Returns a mutable reference to the axis configuration.
    pub fn axis_mut(&mut self) -> &mut AxisConfig {
        &mut self.axis
    }

    /// Sets 2D axis limits manually.
    pub fn set_limits(&mut self, x_min: f64, x_max: f64, y_min: f64, y_max: f64) {
        self.axis.x_min = x_min;
        self.axis.x_max = x_max;
        self.axis.y_min = y_min;
        self.axis.y_max = y_max;
    }

    /// Sets 3D axis limits manually.
    pub fn set_limits3(&mut self, x_min: f64, x_max: f64, y_min: f64, y_max: f64, z_min: f64, z_max: f64) {
        self.axis.x_min = x_min;
        self.axis.x_max = x_max;
        self.axis.y_min = y_min;
        self.axis.y_max = y_max;
        self.axis.z_min = z_min;
        self.axis.z_max = z_max;
    }

    /// Sets the X-axis scale (linear or logarithmic).
    pub fn set_xscale(&mut self, scale: AxisScale) {
        self.axis.x_scale = scale;
    }

    /// Sets the Y-axis scale (linear or logarithmic).
    pub fn set_yscale(&mut self, scale: AxisScale) {
        self.axis.y_scale = scale;
    }

    /// Automatically scales axes to fit all plotted data.
    ///
    /// Analyzes all series, bars, histograms, and surfaces to determine
    /// appropriate axis limits with 5% padding.
    pub fn auto_scale(&mut self) {
        if self.series.is_empty()
            && self.bars.is_empty()
            && self.histogram_bins.is_empty()
            && self.surfaces.is_empty()
        {
            return;
        }

        let mut x_min = f64::MAX;
        let mut x_max = f64::MIN;
        let mut y_min = f64::MAX;
        let mut y_max = f64::MIN;
        let mut z_min = f64::MAX;
        let mut z_max = f64::MIN;

        let mut has_z = false;

        for s in &self.series {
            for &x in &s.x {
                x_min = x_min.min(x);
                x_max = x_max.max(x);
            }
            for &y in &s.y {
                y_min = y_min.min(y);
                y_max = y_max.max(y);
            }
            if !s.z.is_empty() {
                has_z = true;
                for &z in &s.z {
                    z_min = z_min.min(z);
                    z_max = z_max.max(z);
                }
            }
        }

        for surf in &self.surfaces {
            has_z = true;
            for &x in &surf.x {
                x_min = x_min.min(x);
                x_max = x_max.max(x);
            }
            for &y in &surf.y {
                y_min = y_min.min(y);
                y_max = y_max.max(y);
            }
            for &z in &surf.z {
                z_min = z_min.min(z);
                z_max = z_max.max(z);
            }
        }

        for (values, _) in &self.bars {
            x_min = 0.0;
            x_max = x_max.max(values.len() as f64);
            for &v in values {
                y_min = y_min.min(0.0);
                y_max = y_max.max(v);
            }
        }

        for (xs, xe, h, _) in &self.histogram_bins {
            x_min = x_min.min(*xs);
            x_max = x_max.max(*xe);
            y_min = 0.0;
            y_max = y_max.max(*h);
        }

        let x_pad = (x_max - x_min) * 0.05;
        let y_pad = (y_max - y_min) * 0.05;

        self.axis.x_min = x_min - x_pad;
        self.axis.x_max = x_max + x_pad;
        self.axis.y_min = y_min - y_pad;
        self.axis.y_max = y_max + y_pad;

        if has_z {
            // Tight fit for Z to avoid "floating" gap above floor
            self.axis.z_min = z_min;
            self.axis.z_max = z_max;
        }
    }

    /// Creates a 2D line plot.
    ///
    /// # Arguments
    ///
    /// * `x` - X coordinates
    /// * `y` - Y coordinates  
    /// * `color` - Line color (RGBA)
    pub fn plot(&mut self, x: Vec<f64>, y: Vec<f64>, color: Vec4) {
        self.series.push(Series {
            x,
            y,
            color,
            ..Default::default()
        });
    }

    /// Creates a styled 2D line plot with custom line width and style.
    pub fn plot_styled(
        &mut self,
        x: Vec<f64>,
        y: Vec<f64>,
        color: Vec4,
        line_width: f32,
        line_style: LineStyle,
    ) {
        self.series.push(Series {
            x,
            y,
            color,
            line_width,
            line_style,
            ..Default::default()
        });
    }

    /// Creates a log-log plot (both axes logarithmic).
    pub fn loglog(&mut self, x: Vec<f64>, y: Vec<f64>, color: Vec4) {
        self.axis.x_scale = AxisScale::Log10;
        self.axis.y_scale = AxisScale::Log10;
        self.plot(x, y, color);
    }

    /// Creates a semi-log plot (X-axis logarithmic, Y-axis linear).
    pub fn semilogx(&mut self, x: Vec<f64>, y: Vec<f64>, color: Vec4) {
        self.axis.x_scale = AxisScale::Log10;
        self.axis.y_scale = AxisScale::Linear;
        self.plot(x, y, color);
    }

    /// Creates a semi-log plot (X-axis linear, Y-axis logarithmic).
    pub fn semilogy(&mut self, x: Vec<f64>, y: Vec<f64>, color: Vec4) {
        self.axis.x_scale = AxisScale::Linear;
        self.axis.y_scale = AxisScale::Log10;
        self.plot(x, y, color);
    }

    /// Creates a 3D line plot.
    pub fn plot3(&mut self, x: Vec<f64>, y: Vec<f64>, z: Vec<f64>, color: Vec4) {
        self.series.push(Series {
            x,
            y,
            z,
            color,
            ..Default::default()
        });
    }

    /// Creates a styled 3D line plot.
    pub fn plot3_styled(
        &mut self,
        x: Vec<f64>,
        y: Vec<f64>,
        z: Vec<f64>,
        color: Vec4,
        line_width: f32,
        line_style: LineStyle,
    ) {
        self.series.push(Series {
            x,
            y,
            z,
            color,
            line_width,
            line_style,
            ..Default::default()
        });
    }

    /// Creates a 2D scatter plot with circular markers.
    pub fn scatter(&mut self, x: Vec<f64>, y: Vec<f64>, color: Vec4, marker_size: f32) {
        self.series.push(Series {
            x,
            y,
            color,
            line_width: 0.0,
            line_style: LineStyle::Solid,
            marker_style: Some(MarkerStyle::Circle),
            marker_size,
            ..Default::default()
        });
    }

    /// Creates a scatter plot with custom marker style.
    pub fn scatter_styled(
        &mut self,
        x: Vec<f64>,
        y: Vec<f64>,
        color: Vec4,
        marker_size: f32,
        marker_style: MarkerStyle,
    ) {
        self.series.push(Series {
            x,
            y,
            color,
            line_width: 0.0,
            line_style: LineStyle::Solid,
            marker_style: Some(marker_style),
            marker_size,
            ..Default::default()
        });
    }

    /// Creates a 3D scatter plot.
    pub fn scatter3(
        &mut self,
        x: Vec<f64>,
        y: Vec<f64>,
        z: Vec<f64>,
        color: Vec4,
        marker_size: f32,
    ) {
        self.series.push(Series {
            x,
            y,
            z,
            color,
            line_width: 0.0,
            line_style: LineStyle::Solid,
            marker_style: Some(MarkerStyle::Circle),
            marker_size,
            ..Default::default()
        });
    }

    /// Creates a plot with vertical error bars.
    ///
    /// # Arguments
    ///
    /// * `x` - X coordinates
    /// * `y` - Y coordinates
    /// * `y_err` - Symmetric Y error values
    /// * `color` - Line/marker color
    pub fn errorbar(
        &mut self,
        x: Vec<f64>,
        y: Vec<f64>,
        y_err: Vec<f64>,
        color: Vec4,
    ) {
        self.series.push(Series {
            x,
            y,
            y_error: ErrorData::Symmetric { values: y_err },
            color,
            marker_style: Some(MarkerStyle::Circle),
            ..Default::default()
        });
    }

    /// Creates a plot with both horizontal and vertical error bars.
    pub fn errorbar_xy(
        &mut self,
        x: Vec<f64>,
        y: Vec<f64>,
        x_err: Vec<f64>,
        y_err: Vec<f64>,
        color: Vec4,
    ) {
        self.series.push(Series {
            x,
            y,
            x_error: ErrorData::Symmetric { values: x_err },
            y_error: ErrorData::Symmetric { values: y_err },
            color,
            marker_style: Some(MarkerStyle::Circle),
            ..Default::default()
        });
    }

    /// Creates a plot with asymmetric vertical error bars.
    pub fn errorbar_asymmetric(
        &mut self,
        x: Vec<f64>,
        y: Vec<f64>,
        y_err_low: Vec<f64>,
        y_err_high: Vec<f64>,
        color: Vec4,
    ) {
        self.series.push(Series {
            x,
            y,
            y_error: ErrorData::Asymmetric {
                low: y_err_low,
                high: y_err_high,
            },
            color,
            marker_style: Some(MarkerStyle::Circle),
            ..Default::default()
        });
    }

    /// Creates a 3D surface plot with solid color.
    pub fn surface(&mut self, x: Vec<f64>, y: Vec<f64>, z: Vec<f64>, color: Vec4) {
        self.surfaces.push(Surface { x, y, z, color, use_colormap: false });
    }

    /// Creates a 3D surface plot with automatic colormap based on Z values.
    pub fn surface_with_colormap(&mut self, x: Vec<f64>, y: Vec<f64>, z: Vec<f64>) {
        self.surfaces.push(Surface { x, y, z, color: Vec4::new(0.0, 0.0, 0.0, 1.0), use_colormap: true });
    }

    /// Adds a pre-configured surface to the plot.
    pub fn add_surface(&mut self, surface: Surface) {
        self.surfaces.push(surface);
    }

    /// Returns a mutable reference to the series list for direct manipulation.
    pub fn series_mut(&mut self) -> &mut Vec<Series> {
        &mut self.series
    }

    /// Creates a bar chart.
    ///
    /// # Arguments
    ///
    /// * `values` - Bar heights
    /// * `color` - Bar color
    pub fn bar(&mut self, values: Vec<f64>, color: Vec4) {
        self.bars.push((values, color));
    }

    /// Creates a histogram from raw data.
    ///
    /// Automatically bins the data and creates a bar chart visualization.
    ///
    /// # Arguments
    ///
    /// * `data` - Raw data points
    /// * `num_bins` - Number of histogram bins
    /// * `color` - Histogram color
    pub fn histogram(&mut self, data: &[f64], num_bins: usize, color: Vec4) {
        if data.is_empty() || num_bins == 0 {
            return;
        }

        let min_val = data.iter().cloned().fold(f64::MAX, f64::min);
        let max_val = data.iter().cloned().fold(f64::MIN, f64::max);
        let bin_width = (max_val - min_val) / num_bins as f64;

        let mut counts = vec![0usize; num_bins];
        for &v in data {
            let idx = ((v - min_val) / bin_width) as usize;
            let idx = idx.min(num_bins - 1);
            counts[idx] += 1;
        }

        self.histogram_bins.clear();
        for (i, &count) in counts.iter().enumerate() {
            let x_start = min_val + i as f64 * bin_width;
            let x_end = x_start + bin_width;
            self.histogram_bins.push((x_start, x_end, count as f64, color));
        }
    }

    /// Creates an area plot (filled region under a curve).
    ///
    /// # Arguments
    ///
    /// * `x` - X coordinates
    /// * `y` - Y coordinates
    /// * `color` - Fill color (use alpha channel for transparency)
    pub fn area(&mut self, x: Vec<f64>, y: Vec<f64>, color: Vec4) {
        self.areas.push(AreaSeries {
            x,
            y,
            color,
            ..Default::default()
        });
    }

    /// Creates an area plot with custom baseline.
    pub fn area_with_baseline(
        &mut self,
        x: Vec<f64>,
        y: Vec<f64>,
        baseline: f64,
        color: Vec4,
    ) {
        self.areas.push(AreaSeries {
            x,
            y,
            color,
            baseline,
            alpha: color.w,
        });
    }

    fn data_to_screen(&self, x: f64, y: f64) -> Vec2 {
        let plot_width = self.width as f32 - self.margin_left - self.margin_right;
        let plot_height = self.height as f32 - self.margin_top - self.margin_bottom;

        let x_range = self.axis.x_max - self.axis.x_min;
        let y_range = self.axis.y_max - self.axis.y_min;

        let nx = if x_range.abs() > 1e-9 {
            (x - self.axis.x_min) / x_range
        } else {
            0.5
        };
        let ny = if y_range.abs() > 1e-9 {
            (y - self.axis.y_min) / y_range
        } else {
            0.5
        };

        let sx = self.margin_left + nx as f32 * plot_width;
        let sy = self.margin_top + (1.0 - ny as f32) * plot_height;

        Vec2::new(sx, sy)
    }

    fn data_to_pos(&self, x: f64, y: f64, z: Option<f64>, is_3d: bool) -> glam::Vec3 {
        if is_3d {
            let z_val = z.unwrap_or(0.0);
            let x_range = self.axis.x_max - self.axis.x_min;
            let y_range = self.axis.y_max - self.axis.y_min;
            let z_range = self.axis.z_max - self.axis.z_min;

            let nx = if x_range.abs() > 1e-9 { (x - self.axis.x_min) / x_range } else { 0.5 };
            let ny = if y_range.abs() > 1e-9 { (y - self.axis.y_min) / y_range } else { 0.5 };
            let nz = if z_range.abs() > 1e-9 { (z_val - self.axis.z_min) / z_range } else { 0.5 };

            // Map to [-1, 1] for 3D world space
            glam::Vec3::new(nx as f32 * 2.0 - 1.0, ny as f32 * 2.0 - 1.0, nz as f32 * 2.0 - 1.0)
        } else {
            self.data_to_screen(x, y).extend(0.0)
        }
    }

    pub fn render(
        &self,
        renderer: &mut PrimitiveRenderer,
        text_renderer: &mut crate::text::TextRenderer,
        view_proj: Option<glam::Mat4>,
    ) {
        let is_3d = self.series.iter().any(|s| !s.z.is_empty()) || !self.surfaces.is_empty();
        let plot_width = self.width as f32 - self.margin_left - self.margin_right;
        let plot_height = self.height as f32 - self.margin_top - self.margin_bottom;

        // 1. Solid Underlays
        if !is_3d {
            renderer.draw_rect(
                Vec2::new(
                    self.margin_left + plot_width / 2.0,
                    self.margin_top + plot_height / 2.0,
                ),
                Vec2::new(plot_width, plot_height),
                self.background_color,
                0.0,
                0.0,
            );
        } else {
            // Draw all surface solid faces first
            for surf in &self.surfaces {
                self.draw_surface_faces(renderer, surf);
            }
        }



        // 2. Grid (Background) and 3D Frame
        if !is_3d {
            if self.axis.show_grid {
                self.draw_grid(renderer);
            }
        } else {
            self.draw_3d_box_and_walls(renderer);
        }

        // 3. Area Plots (background fill)
        if !is_3d {
            for area in &self.areas {
                self.draw_area(renderer, area);
            }
        }

        // 4. Error Bars (before series so markers appear on top)
        if !is_3d {
            for series in &self.series {
                self.draw_error_bars(renderer, series);
            }
        }

        // 4. Plotted Data (Series)
        for series in &self.series {
            self.draw_series(renderer, series, is_3d);
        }

        // 4. Axes and Labels (Foreground - 2D only)
        if !is_3d {
            self.draw_axes_border(renderer);
            self.draw_ticks_and_labels(renderer, text_renderer);
        }

        // 4. Surface Mesh Lines (last to blend nicely over faces)
        if is_3d {
            for surf in &self.surfaces {
                self.draw_surface_lines(renderer, surf);
            }
        }

        // 5. Annotations and 3D Labels
        if is_3d {
            if let Some(vp) = view_proj {
                self.draw_3d_ticks_and_labels(renderer, text_renderer, vp);
            }
        }

        if !is_3d {
            for (values, color) in &self.bars {
                self.draw_bars(renderer, values, *color);
            }
            self.draw_histogram_bins(renderer);
        }

        // 6. Title
        if !self.axis.title.is_empty() {
            let title_size = 32.0 * self.scale_factor;
            let title_width = text_renderer.measure_text(&self.axis.title, title_size).x;
            text_renderer.draw_text(
                &self.axis.title,
                Vec2::new(
                    self.margin_left + (plot_width - title_width) / 2.0,
                    self.margin_top - 40.0 * self.scale_factor,
                ),
                title_size,
                Vec4::new(0.0, 0.0, 0.0, 1.0),
            );
        }
    }

    fn draw_grid(&self, renderer: &mut PrimitiveRenderer) {
        let grid_color = Vec4::new(0.8, 0.8, 0.8, 1.0);
        let num_ticks = 10;
        let x_range = self.axis.x_max - self.axis.x_min;
        let y_range = self.axis.y_max - self.axis.y_min;
        let grid_width = 1.0 * self.scale_factor;

        for i in 0..=num_ticks {
            let t = i as f64 / num_ticks as f64;
            let x = self.axis.x_min + t * x_range;
            let top = self.data_to_screen(x, self.axis.y_max);
            let bottom = self.data_to_screen(x, self.axis.y_min);
            renderer.draw_line(
                top.extend(0.0),
                bottom.extend(0.0),
                grid_width,
                grid_color,
                0.0,
                0.0,
                0.0,
            );
        }

        for i in 0..=num_ticks {
            let t = i as f64 / num_ticks as f64;
            let y = self.axis.y_min + t * y_range;
            let left = self.data_to_screen(self.axis.x_min, y);
            let right = self.data_to_screen(self.axis.x_max, y);
            renderer.draw_line(
                left.extend(0.0),
                right.extend(0.0),
                grid_width,
                grid_color,
                0.0,
                0.0,
                0.0,
            );
        }
    }

    fn draw_ticks_and_labels(
        &self,
        renderer: &mut PrimitiveRenderer,
        text_renderer: &mut crate::text::TextRenderer,
    ) {
        let tick_color = Vec4::new(0.0, 0.0, 0.0, 1.0);
        let num_ticks = 10;
        let x_range = self.axis.x_max - self.axis.x_min;
        let y_range = self.axis.y_max - self.axis.y_min;
        let tick_size = 10.0 * self.scale_factor;
        let label_size = 22.0 * self.scale_factor;
        let tick_width = 2.0 * self.scale_factor;

        // X axis ticks
        for i in 0..=num_ticks {
            let t = i as f64 / num_ticks as f64;
            let x = self.axis.x_min + t * x_range;
            let p = self.data_to_screen(x, self.axis.y_min);
            renderer.draw_line(
                p.extend(0.0),
                (p + Vec2::new(0.0, tick_size)).extend(0.0),
                tick_width,
                tick_color,
                0.0,
                0.0,
                0.0,
            );

            let label = format!("{:.1}", x);
            let label_width = text_renderer.measure_text(&label, label_size).x;
            text_renderer.draw_text(
                &label,
                p + Vec2::new(-label_width / 2.0, tick_size + 5.0 * self.scale_factor),
                label_size,
                tick_color,
            );
        }

        // Y axis ticks
        for i in 0..=num_ticks {
            let t = i as f64 / num_ticks as f64;
            let y = self.axis.y_min + t * y_range;
            let p = self.data_to_screen(self.axis.x_min, y);
            renderer.draw_line(
                p.extend(0.0),
                (p - Vec2::new(tick_size, 0.0)).extend(0.0),
                tick_width,
                tick_color,
                0.0,
                0.0,
                0.0,
            );

            let label = format!("{:.1}", y);
            let label_dim = text_renderer.measure_text(&label, label_size);
            text_renderer.draw_text(
                &label,
                p - Vec2::new(label_dim.x + tick_size + 5.0 * self.scale_factor, label_dim.y / 2.0),
                label_size,
                tick_color,
            );
        }
    }

    fn draw_axes_border(&self, renderer: &mut PrimitiveRenderer) {
        let border_color = Vec4::new(0.0, 0.0, 0.0, 1.0);
        let border_width = 2.0 * self.scale_factor;
        let tl = self.data_to_screen(self.axis.x_min, self.axis.y_max);
        let tr = self.data_to_screen(self.axis.x_max, self.axis.y_max);
        let bl = self.data_to_screen(self.axis.x_min, self.axis.y_min);
        let br = self.data_to_screen(self.axis.x_max, self.axis.y_min);

        renderer.draw_line(
            tl.extend(0.0),
            tr.extend(0.0),
            border_width,
            border_color,
            0.0,
            0.0,
            0.0,
        );
        renderer.draw_line(
            bl.extend(0.0),
            br.extend(0.0),
            border_width,
            border_color,
            0.0,
            0.0,
            0.0,
        );
        renderer.draw_line(
            tl.extend(0.0),
            bl.extend(0.0),
            border_width,
            border_color,
            0.0,
            0.0,
            0.0,
        );
        renderer.draw_line(
            tr.extend(0.0),
            br.extend(0.0),
            border_width,
            border_color,
            0.0,
            0.0,
            0.0,
        );
    }

    fn draw_3d_box_and_walls(&self, renderer: &mut PrimitiveRenderer) {
        let wall_color = self.background_color;
        let grid_color = Vec4::new(0.5, 0.5, 0.5, 1.0);
        let grid_width = 0.004 * self.scale_factor;
        let num_ticks = 5;

        // Helper: Quad
        let quad = |rend: &mut PrimitiveRenderer, p0: glam::Vec3, p1: glam::Vec3, p2: glam::Vec3, p3: glam::Vec3| {
            rend.draw_triangle_unlit(p0, p1, p2, wall_color);
            rend.draw_triangle_unlit(p0, p2, p3, wall_color);
        };

        let min = -1.0;
        let max = 1.0;
        // Align depths. Walls at EXACTLY -1.0. Lines/Grid slightly offset INWARD/TOP to separate.
        let d_walls = -1.0;
        let d_grid = -1.0 + 0.002; 

        // 1. Floor (Z = min)
        quad(
            renderer,
            glam::Vec3::new(min, max, d_walls),
            glam::Vec3::new(max, max, d_walls),
            glam::Vec3::new(max, min, d_walls),
            glam::Vec3::new(min, min, d_walls),
        );
        for i in 0..=num_ticks {
            let t = -1.0 + 2.0 * (i as f32 / num_ticks as f32);
            renderer.draw_line(glam::Vec3::new(min, t, d_grid), glam::Vec3::new(max, t, d_grid), grid_width, grid_color, 0.0, 0.0, 0.0);
            renderer.draw_line(glam::Vec3::new(t, min, d_grid), glam::Vec3::new(t, max, d_grid), grid_width, grid_color, 0.0, 0.0, 0.0);
        }

        // 2. Back Wall (X = min)
        quad(
            renderer,
            glam::Vec3::new(d_walls, max, max),
            glam::Vec3::new(d_walls, max, min),
            glam::Vec3::new(d_walls, min, min),
            glam::Vec3::new(d_walls, min, max),
        );
        for i in 0..=num_ticks {
            let t = -1.0 + 2.0 * (i as f32 / num_ticks as f32);
            renderer.draw_line(glam::Vec3::new(d_grid, t, min), glam::Vec3::new(d_grid, t, max), grid_width, grid_color, 0.0, 0.0, 0.0);
            renderer.draw_line(glam::Vec3::new(d_grid, min, t), glam::Vec3::new(d_grid, max, t), grid_width, grid_color, 0.0, 0.0, 0.0);
        }

        // 3. Side Wall (Y = min)
        quad(
            renderer,
            glam::Vec3::new(min, d_walls, max),
            glam::Vec3::new(min, d_walls, min),
            glam::Vec3::new(max, d_walls, min),
            glam::Vec3::new(max, d_walls, max),
        );
        for i in 0..=num_ticks {
            let t = -1.0 + 2.0 * (i as f32 / num_ticks as f32);
            renderer.draw_line(glam::Vec3::new(t, d_grid, min), glam::Vec3::new(t, d_grid, max), grid_width, grid_color, 0.0, 0.0, 0.0);
            renderer.draw_line(glam::Vec3::new(min, d_grid, t), glam::Vec3::new(max, d_grid, t), grid_width, grid_color, 0.0, 0.0, 0.0);
        }
        
        // 4. Open Frame (Floor + Pillars)
        // User requested "Open Box". We draw the outline of the 3 walls.
        let border_color = Vec4::new(0.0, 0.0, 0.0, 1.0);
        let bw = 0.008 * self.scale_factor;
        
        // Floor Outline
        let p00 = glam::Vec3::new(min, min, d_grid);
        let p10 = glam::Vec3::new(max, min, d_grid);
        let p11 = glam::Vec3::new(max, max, d_grid);
        let p01 = glam::Vec3::new(min, max, d_grid);
        renderer.draw_line(p00, p10, bw, border_color, 0.0, 0.0, 0.0);
        renderer.draw_line(p10, p11, bw, border_color, 0.0, 0.0, 0.0);
        renderer.draw_line(p11, p01, bw, border_color, 0.0, 0.0, 0.0);
        renderer.draw_line(p01, p00, bw, border_color, 0.0, 0.0, 0.0);

        // Vertical Pillars at corners (Back)
        renderer.draw_line(glam::Vec3::new(min, min, min), glam::Vec3::new(min, min, max), bw, border_color, 0.0, 0.0, 0.0); // Corner
        renderer.draw_line(glam::Vec3::new(max, min, min), glam::Vec3::new(max, min, max), bw, border_color, 0.0, 0.0, 0.0); // Right Back
        renderer.draw_line(glam::Vec3::new(min, max, min), glam::Vec3::new(min, max, max), bw, border_color, 0.0, 0.0, 0.0); // Left Back
        
        // Top Edges of Walls
        renderer.draw_line(glam::Vec3::new(min, min, max), glam::Vec3::new(max, min, max), bw, border_color, 0.0, 0.0, 0.0); // Side Top
        renderer.draw_line(glam::Vec3::new(min, min, max), glam::Vec3::new(min, max, max), bw, border_color, 0.0, 0.0, 0.0); // Back Top
        


    }

    fn draw_3d_ticks_and_labels(
        &self,
        renderer: &mut PrimitiveRenderer,
        text_renderer: &mut crate::text::TextRenderer,
        view_proj: glam::Mat4,
    ) {
        let tick_color = Vec4::new(0.0, 0.0, 0.0, 1.0);
        let num_ticks = 5;
        let tick_len = 0.05;
        let label_size = 18.0 * self.scale_factor;
        // Reduced tick width (was 0.012)
        let tick_width = 0.008 * self.scale_factor;

        let project = |p: glam::Vec3| -> Vec2 {
            use glam::Vec4Swizzles;
            let clip = view_proj * p.extend(1.0);
            let ndc = clip.xyz() / clip.w;
            Vec2::new(
                (ndc.x * 0.5 + 0.5) * self.width as f32,
                (0.5 - ndc.y * 0.5) * self.height as f32,
            )
        };

        // Match d_grid from draw_3d_box_and_walls
        let d_grid = -1.0 + 0.002;
        let label_gap = 0.08; // Gap in World Units

        // X axis ticks (Front Edge: y=max, z=d_grid)
        for i in 0..=num_ticks {
            let t = i as f64 / num_ticks as f64;
            let val = self.axis.x_min + t * (self.axis.x_max - self.axis.x_min);
            // Move to y=1.0 (Front), z=d_grid
            let p_world = glam::Vec3::new(-1.0 + 2.0 * t as f32, 1.0, d_grid);
            let p_tick = p_world + glam::Vec3::new(0.0, tick_len, 0.0); // Point +Y (Out)
            let p_label = p_tick + glam::Vec3::new(0.0, label_gap, 0.0); // Label further +Y
            
            renderer.draw_line(p_world, p_tick, tick_width, tick_color, 0.0, 0.0, 0.0);
            
            let screen_pos = project(p_label);
            let label = format!("{:.1}", val);
            let label_dim = text_renderer.measure_text(&label, label_size);
            // Center horizontally, push down slightly? 
            // 3D placement does the heavy work. Just center text on the point.
            text_renderer.draw_text(&label, screen_pos - Vec2::new(label_dim.x / 2.0, label_dim.y / 2.0), label_size, tick_color);
        }

        // Y axis ticks (Right Edge: x=max, z=d_grid)
        for i in 0..=num_ticks {
            let t = i as f64 / num_ticks as f64;
            let val = self.axis.y_min + t * (self.axis.y_max - self.axis.y_min);
            // Move to x=1.0 (Right), z=d_grid
            let p_world = glam::Vec3::new(1.0, -1.0 + 2.0 * t as f32, d_grid); 
            
            let p_tick = p_world + glam::Vec3::new(tick_len, 0.0, 0.0); // Point +X (Right, Out)
            let p_label = p_tick + glam::Vec3::new(label_gap, 0.0, 0.0); // Label further +X
            
            renderer.draw_line(p_world, p_tick, tick_width, tick_color, 0.0, 0.0, 0.0);
            
            let screen_pos = project(p_label);
            let label = format!("{:.1}", val);
            let label_dim = text_renderer.measure_text(&label, label_size);
            // Center text on point
            text_renderer.draw_text(&label, screen_pos - Vec2::new(label_dim.x / 2.0, label_dim.y / 2.0), label_size, tick_color);
        }

        // Z axis ticks (Left Wall Edge: x=min, y=max)
        for i in 0..=num_ticks {
            let t = i as f64 / num_ticks as f64;
            let val = self.axis.z_min + t * (self.axis.z_max - self.axis.z_min);
            // Move to x=-1.0, y=1.0 (Left, Back-ish)
            let p_world = glam::Vec3::new(-1.0, 1.0, -1.0 + 2.0 * t as f32);
            // Point +Y (Right/Outwards in screen space projection)
            let p_tick = p_world + glam::Vec3::new(0.0, tick_len, 0.0); 
            let p_label = p_tick + glam::Vec3::new(0.0, label_gap, 0.0); // Label further +Y
            
            renderer.draw_line(p_world, p_tick, tick_width, tick_color, 0.0, 0.0, 0.0);
            
            let screen_pos = project(p_label);
            let label = format!("{:.1}", val);
            let label_dim = text_renderer.measure_text(&label, label_size);
            // Center text on point
            text_renderer.draw_text(&label, screen_pos - Vec2::new(label_dim.x / 2.0, label_dim.y / 2.0), label_size, tick_color);
        }
    }

    fn draw_series(&self, renderer: &mut PrimitiveRenderer, series: &Series, is_3d: bool) {
        if series.x.len() < 2 || series.y.len() < 2 {
            return;
        }

        let count = series.x.len().min(series.y.len()).min(if is_3d { series.z.len() } else { usize::MAX });
        let (dash_len, gap_len) = match series.line_style {
            LineStyle::Solid => (0.0, 0.0),
            LineStyle::Dashed => (8.0, 4.0),
            LineStyle::Dotted => (2.0, 4.0),
        };

        if series.line_width > 0.0 {
            for i in 0..count - 1 {
                let p0 = self.data_to_pos(series.x[i], series.y[i], if is_3d { Some(series.z[i]) } else { None }, is_3d);
                let p1 = self.data_to_pos(series.x[i + 1], series.y[i + 1], if is_3d { Some(series.z[i + 1]) } else { None }, is_3d);
                
                renderer.draw_line(
                    p0,
                    p1,
                    if is_3d { series.line_width * 0.005 * self.scale_factor } else { series.line_width * self.scale_factor },
                    series.color,
                    dash_len * self.scale_factor,
                    gap_len * self.scale_factor,
                    0.0,
                );
            }
        }

        if let Some(marker_style) = series.marker_style {
            let prim_type = match marker_style {
                MarkerStyle::Circle => 1,
                MarkerStyle::Square => 0,
                MarkerStyle::Diamond => 13,
                MarkerStyle::Plus => 10,
                MarkerStyle::Cross => 11,
                MarkerStyle::Point => 1,
            };
            for i in 0..count {
                let p = self.data_to_pos(series.x[i], series.y[i], if is_3d { Some(series.z[i]) } else { None }, is_3d);
                renderer.draw_circle(
                    p,
                    if is_3d { series.marker_size * 0.005 * self.scale_factor } else { series.marker_size * 0.5 * self.scale_factor },
                    series.color,
                    0.0,
                    prim_type,
                );
            }
        }
    }

    fn draw_bars(&self, renderer: &mut PrimitiveRenderer, values: &[f64], color: Vec4) {
        if values.is_empty() {
            return;
        }

        let num_bars = values.len();
        let bar_spacing = 0.2;
        let bar_width = (1.0 - bar_spacing) / num_bars as f64;

        for (i, &val) in values.iter().enumerate() {
            let x_center = i as f64 + 0.5;
            let x_left = x_center - bar_width / 2.0;
            let x_right = x_center + bar_width / 2.0;

            let tl = self.data_to_screen(x_left, val);
            let br = self.data_to_screen(x_right, 0.0);

            let center = (tl + br) / 2.0;
            let half_size = Vec2::new((br.x - tl.x).abs() / 2.0, (br.y - tl.y).abs() / 2.0);

            renderer.draw_rect(center, half_size * 2.0, color, 0.0, 0.0);
        }
    }

    fn draw_histogram_bins(&self, renderer: &mut PrimitiveRenderer) {
        for (x_start, x_end, height, color) in &self.histogram_bins {
            let tl = self.data_to_screen(*x_start, *height);
            let br = self.data_to_screen(*x_end, 0.0);

            let center = (tl + br) / 2.0;
            let half_size = Vec2::new((br.x - tl.x).abs() / 2.0, (br.y - tl.y).abs() / 2.0);

            renderer.draw_rect(center, half_size * 2.0, *color, 0.0, 0.0);
        }
    }

    fn draw_error_bars(
        &self,
        renderer: &mut PrimitiveRenderer,
        series: &Series,
    ) {
        if !series.error_style.enabled {
            return;
        }

        let count = series.x.len().min(series.y.len());
        let err_color = series.color;
        let line_width = series.error_style.line_width * self.scale_factor;
        let cap_width = series.error_style.cap_width * self.scale_factor;

        // Draw Y error bars
        match &series.y_error {
            ErrorData::Symmetric { values } => {
                for i in 0..count.min(values.len()) {
                    let x = series.x[i];
                    let y = series.y[i];
                    let err = values[i];

                    let top = self.data_to_screen(x, y + err);
                    let bottom = self.data_to_screen(x, y - err);

                    // Vertical line
                    renderer.draw_line(
                        top.extend(0.0),
                        bottom.extend(0.0),
                        line_width,
                        err_color,
                        0.0,
                        0.0,
                        0.0,
                    );

                    // Top cap
                    if cap_width > 0.0 {
                        renderer.draw_line(
                            (top - Vec2::new(cap_width / 2.0, 0.0)).extend(0.0),
                            (top + Vec2::new(cap_width / 2.0, 0.0)).extend(0.0),
                            line_width,
                            err_color,
                            0.0,
                            0.0,
                            0.0,
                        );
                    }

                    // Bottom cap
                    if cap_width > 0.0 {
                        renderer.draw_line(
                            (bottom - Vec2::new(cap_width / 2.0, 0.0)).extend(0.0),
                            (bottom + Vec2::new(cap_width / 2.0, 0.0)).extend(0.0),
                            line_width,
                            err_color,
                            0.0,
                            0.0,
                            0.0,
                        );
                    }
                }
            }
            ErrorData::Asymmetric { low, high } => {
                for i in 0..count.min(low.len()).min(high.len()) {
                    let x = series.x[i];
                    let y = series.y[i];
                    let err_low = low[i];
                    let err_high = high[i];

                    let top = self.data_to_screen(x, y + err_high);
                    let bottom = self.data_to_screen(x, y - err_low);

                    // Vertical line
                    renderer.draw_line(
                        top.extend(0.0),
                        bottom.extend(0.0),
                        line_width,
                        err_color,
                        0.0,
                        0.0,
                        0.0,
                    );

                    // Top cap
                    if cap_width > 0.0 {
                        renderer.draw_line(
                            (top - Vec2::new(cap_width / 2.0, 0.0)).extend(0.0),
                            (top + Vec2::new(cap_width / 2.0, 0.0)).extend(0.0),
                            line_width,
                            err_color,
                            0.0,
                            0.0,
                            0.0,
                        );
                    }

                    // Bottom cap
                    if cap_width > 0.0 {
                        renderer.draw_line(
                            (bottom - Vec2::new(cap_width / 2.0, 0.0)).extend(0.0),
                            (bottom + Vec2::new(cap_width / 2.0, 0.0)).extend(0.0),
                            line_width,
                            err_color,
                            0.0,
                            0.0,
                            0.0,
                        );
                    }
                }
            }
            ErrorData::None => {}
        }

        // Draw X error bars
        match &series.x_error {
            ErrorData::Symmetric { values } => {
                for i in 0..count.min(values.len()) {
                    let x = series.x[i];
                    let y = series.y[i];
                    let err = values[i];

                    let left = self.data_to_screen(x - err, y);
                    let right = self.data_to_screen(x + err, y);

                    // Horizontal line
                    renderer.draw_line(
                        left.extend(0.0),
                        right.extend(0.0),
                        line_width,
                        err_color,
                        0.0,
                        0.0,
                        0.0,
                    );

                    // Left cap
                    if cap_width > 0.0 {
                        renderer.draw_line(
                            (left - Vec2::new(0.0, cap_width / 2.0)).extend(0.0),
                            (left + Vec2::new(0.0, cap_width / 2.0)).extend(0.0),
                            line_width,
                            err_color,
                            0.0,
                            0.0,
                            0.0,
                        );
                    }

                    // Right cap
                    if cap_width > 0.0 {
                        renderer.draw_line(
                            (right - Vec2::new(0.0, cap_width / 2.0)).extend(0.0),
                            (right + Vec2::new(0.0, cap_width / 2.0)).extend(0.0),
                            line_width,
                            err_color,
                            0.0,
                            0.0,
                            0.0,
                        );
                    }
                }
            }
            ErrorData::Asymmetric { low, high } => {
                for i in 0..count.min(low.len()).min(high.len()) {
                    let x = series.x[i];
                    let y = series.y[i];
                    let err_low = low[i];
                    let err_high = high[i];

                    let left = self.data_to_screen(x - err_low, y);
                    let right = self.data_to_screen(x + err_high, y);

                    // Horizontal line
                    renderer.draw_line(
                        left.extend(0.0),
                        right.extend(0.0),
                        line_width,
                        err_color,
                        0.0,
                        0.0,
                        0.0,
                    );

                    // Left cap
                    if cap_width > 0.0 {
                        renderer.draw_line(
                            (left - Vec2::new(0.0, cap_width / 2.0)).extend(0.0),
                            (left + Vec2::new(0.0, cap_width / 2.0)).extend(0.0),
                            line_width,
                            err_color,
                            0.0,
                            0.0,
                            0.0,
                        );
                    }

                    // Right cap
                    if cap_width > 0.0 {
                        renderer.draw_line(
                            (right - Vec2::new(0.0, cap_width / 2.0)).extend(0.0),
                            (right + Vec2::new(0.0, cap_width / 2.0)).extend(0.0),
                            line_width,
                            err_color,
                            0.0,
                            0.0,
                            0.0,
                        );
                    }
                }
            }
            ErrorData::None => {}
        }
    }

    fn draw_area(&self, renderer: &mut PrimitiveRenderer, area: &AreaSeries) {
        let count = area.x.len().min(area.y.len());
        if count < 2 {
            return;
        }

        // Create fill color with alpha
        let fill_color = Vec4::new(
            area.color.x,
            area.color.y,
            area.color.z,
            area.alpha,
        );

        // Draw filled triangles to create the area under the curve
        for i in 0..count - 1 {
            let x1 = area.x[i];
            let y1 = area.y[i];
            let x2 = area.x[i + 1];
            let y2 = area.y[i + 1];

            let p1 = self.data_to_screen(x1, y1);
            let p2 = self.data_to_screen(x2, y2);
            let base1 = self.data_to_screen(x1, area.baseline);
            let base2 = self.data_to_screen(x2, area.baseline);

            // Create a quad (two triangles) for this segment
            // Triangle 1: p1, p2, base2
            // Triangle 2: p1, base2, base1
            
            // For simplicity, using rectangles to approximate the fill
            // A more sophisticated approach would use proper triangulation
            let center_y = (p1.y + p2.y + base1.y + base2.y) / 4.0;
            let center_x = (p1.x + p2.x + base1.x + base2.x) / 4.0;
            
            let width = (p2.x - p1.x).abs().max(1.0);
            let height = ((p1.y + p2.y) / 2.0 - (base1.y + base2.y) / 2.0).abs();
            
            renderer.draw_rect(
                Vec2::new(center_x, center_y),
                Vec2::new(width, height),
                fill_color,
                0.0,
                0.0,
            );
        }
    }

    fn draw_surface_faces(&self, renderer: &mut PrimitiveRenderer, surface: &Surface) {
        let nx = surface.x.len();
        let ny = surface.y.len();
        if nx < 2 || ny < 2 || surface.z.len() != nx * ny {
            return;
        }

        let face_color = Vec4::new(
            surface.color.x * 0.4,
            surface.color.y * 0.4,
            surface.color.z * 0.4,
            1.0, // Opaque
        );

        // Pre-calculate min/max Z for normalization if using colormap
        let mut z_min = 0.0;
        let mut z_max = 1.0;
        if surface.use_colormap {
             z_min = surface.z.iter().fold(f64::INFINITY, |a, &b| a.min(b));
             z_max = surface.z.iter().fold(f64::NEG_INFINITY, |a, &b| a.max(b));
             if (z_max - z_min).abs() < 1e-6 {
                 z_max = z_min + 1.0;
             }
        }

        for i in 0..nx - 1 {
            for j in 0..ny - 1 {
                let idx00 = i * ny + j;
                let idx10 = (i + 1) * ny + j;
                let idx01 = i * ny + j + 1;
                let idx11 = (i + 1) * ny + j + 1;

                let z00 = surface.z[idx00];
                let z10 = surface.z[idx10];
                let z01 = surface.z[idx01];
                let z11 = surface.z[idx11];

                let p00 = self.data_to_pos(surface.x[i], surface.y[j], Some(z00), true);
                let p10 = self.data_to_pos(surface.x[i+1], surface.y[j], Some(z10), true);
                let p01 = self.data_to_pos(surface.x[i], surface.y[j+1], Some(z01), true);
                let p11 = self.data_to_pos(surface.x[i+1], surface.y[j+1], Some(z11), true);
                
                if surface.use_colormap {
                    // Average Z for the face (or vertex coloring?)
                    // Flat shading implies single color per face.
                    // Tri 1: 00, 10, 11
                    let z_tri1 = (z00 + z10 + z11) / 3.0;
                    let t_tri1 = ((z_tri1 - z_min) / (z_max - z_min)) as f64;
                    let c_tri1 = colormap_viridis(t_tri1);
                    renderer.draw_triangle(p00, p10, p11, c_tri1);
                    
                    // Tri 2: 00, 11, 01
                    let z_tri2 = (z00 + z11 + z01) / 3.0;
                    let t_tri2 = ((z_tri2 - z_min) / (z_max - z_min)) as f64;
                    let c_tri2 = colormap_viridis(t_tri2);
                    renderer.draw_triangle(p00, p11, p01, c_tri2);
                    
                    // Double-sided drawing (Reverse winding)
                    renderer.draw_triangle(p00, p11, p10, c_tri1); 
                    renderer.draw_triangle(p00, p01, p11, c_tri2);
                } else {
                    renderer.draw_triangle(p00, p10, p11, face_color);
                    renderer.draw_triangle(p00, p11, p01, face_color);
                    
                    // Double-sided for solid color too
                    renderer.draw_triangle(p00, p11, p10, face_color); 
                    renderer.draw_triangle(p00, p01, p11, face_color);
                }
            }
        }

    }



    fn draw_surface_lines(&self, renderer: &mut PrimitiveRenderer, surface: &Surface) {
        let nx = surface.x.len();
        let ny = surface.y.len();
        if nx < 2 || ny < 2 || surface.z.len() != nx * ny {
            return;
        }

        // Reduced line width (was 0.015)
        let line_width = 0.006 * self.scale_factor;
        let line_color = surface.color;
        let offset = 0.01; // Maintain small offset

        for i in 0..nx - 1 {
            for j in 0..ny - 1 {
                let p00 = self.data_to_pos(surface.x[i], surface.y[j], Some(surface.z[i * ny + j] + offset), true);
                let p10 = self.data_to_pos(surface.x[i+1], surface.y[j], Some(surface.z[(i+1) * ny + j] + offset), true);
                let p01 = self.data_to_pos(surface.x[i], surface.y[j+1], Some(surface.z[i * ny + j + 1] + offset), true);

                renderer.draw_line(p00, p10, line_width, line_color, 0.0, 0.0, 0.0);
                renderer.draw_line(p00, p01, line_width, line_color, 0.0, 0.0, 0.0);
            }
        }
        
        // Last row/col lines
        for i in 0..nx - 1 {
            let p0 = self.data_to_pos(surface.x[i], surface.y[ny-1], Some(surface.z[i * ny + ny - 1] + offset), true);
            let p1 = self.data_to_pos(surface.x[i+1], surface.y[ny-1], Some(surface.z[(i+1) * ny + ny - 1] + offset), true);
            renderer.draw_line(p0, p1, line_width, line_color, 0.0, 0.0, 0.0);
        }
        for j in 0..ny - 1 {
            let p0 = self.data_to_pos(surface.x[nx-1], surface.y[j], Some(surface.z[(nx-1) * ny + j] + offset), true);
            let p1 = self.data_to_pos(surface.x[nx-1], surface.y[j+1], Some(surface.z[(nx-1) * ny + j + 1] + offset), true);
            renderer.draw_line(p0, p1, line_width, line_color, 0.0, 0.0, 0.0);
        }
    }
}

fn colormap_viridis(t: f64) -> Vec4 {
    let t = t.clamp(0.0, 1.0) as f32;
    // Viridis control points
    let c0 = Vec4::new(0.267, 0.004, 0.329, 1.0); // Purple
    let c1 = Vec4::new(0.190, 0.407, 0.556, 1.0); // Blue
    let c2 = Vec4::new(0.208, 0.718, 0.472, 1.0); // Green
    let c3 = Vec4::new(0.993, 0.906, 0.143, 1.0); // Yellow
    
    if t < 0.33 {
        let local_t = t / 0.33;
        c0.lerp(c1, local_t)
    } else if t < 0.66 {
        let local_t = (t - 0.33) / 0.33;
        c1.lerp(c2, local_t)
    } else {
        let local_t = (t - 0.66) / 0.34;
        c2.lerp(c3, local_t)
    }
}

pub fn linspace(start: f64, end: f64, n: usize) -> Vec<f64> {
    if n <= 1 {
        return vec![start];
    }
    let step = (end - start) / (n - 1) as f64;
    (0..n).map(|i| start + i as f64 * step).collect()
}

pub fn randn(mean: f64, std_dev: f64, n: usize) -> Vec<f64> {
    use std::f64::consts::PI;
    let mut result = Vec::with_capacity(n);
    for i in (0..n).step_by(2) {
        let seed1 = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap()
            .subsec_nanos() as f64
            / 4_294_967_295.0;
        std::thread::sleep(std::time::Duration::from_nanos(1));
        let seed2 = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap()
            .subsec_nanos() as f64
            / 4_294_967_295.0;
        let u1 = seed1.max(1e-10);
        let u2 = seed2;
        let z0 = (-2.0 * u1.ln()).sqrt() * (2.0 * PI * u2).cos();
        let z1 = (-2.0 * u1.ln()).sqrt() * (2.0 * PI * u2).sin();
        result.push(mean + z0 * std_dev);
        if i + 1 < n {
            result.push(mean + z1 * std_dev);
        }
    }
    result
}

pub const PI: f64 = std::f64::consts::PI;
