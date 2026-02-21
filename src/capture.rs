// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

//! Headless off-screen rendering and PNG capture.
//!
//! Provides [`HeadlessRenderer`] for rendering to an off-screen wgpu
//! texture and reading back pixels, and [`PlotCapture`] which combines
//! headless rendering with the matplot++ plotting pipeline.

use crate::plotting::PlotBackend;
use crate::primitives::PrimitiveRenderer;
use crate::text::TextRenderer;
use std::path::Path;

/// wgpu's required row alignment for buffer-to-texture copies.
const COPY_BYTES_PER_ROW_ALIGNMENT: u32 = 256;

/// Default font path on Linux systems.
const DEFAULT_FONT_PATH: &str =
  "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";

/// Off-screen texture format used for capture.
const CAPTURE_FORMAT: wgpu::TextureFormat =
  wgpu::TextureFormat::Rgba8UnormSrgb;

/// Computes the padded bytes-per-row for a given width.
fn padded_bytes_per_row(width: u32) -> u32 {
  let unpadded = width * 4;
  let align = COPY_BYTES_PER_ROW_ALIGNMENT;
  (unpadded + align - 1) / align * align
}

/// Loads font data from the default system path.
fn load_default_font() -> Vec<u8> {
  std::fs::read(DEFAULT_FONT_PATH).unwrap_or_else(|e| {
    panic!(
      "Failed to load font from {}: {}",
      DEFAULT_FONT_PATH, e
    );
  })
}

/// Headless wgpu renderer for off-screen capture.
///
/// Creates its own adapter, device, and queue without a surface,
/// renders to an off-screen texture, and reads back RGBA pixels.
pub struct HeadlessRenderer {
  device: wgpu::Device,
  queue: wgpu::Queue,
  texture: wgpu::Texture,
  staging_buffer: wgpu::Buffer,
  prim: PrimitiveRenderer,
  text: TextRenderer,
  width: u32,
  height: u32,
}

impl HeadlessRenderer {
  /// Creates a new headless renderer with the given dimensions.
  ///
  /// Requests a wgpu adapter without a compatible surface, creates
  /// the device/queue, off-screen texture, staging buffer, and both
  /// primitive and text renderers.
  pub fn new(width: u32, height: u32) -> Self {
    let instance = wgpu::Instance::new(wgpu::InstanceDescriptor {
      backends: wgpu::Backends::all(),
      ..Default::default()
    });

    let adapter = pollster::block_on(
      instance.request_adapter(&wgpu::RequestAdapterOptions {
        power_preference: wgpu::PowerPreference::default(),
        compatible_surface: None,
        force_fallback_adapter: false,
      }),
    )
    .expect("Failed to find a suitable GPU adapter");

    let (device, queue) = pollster::block_on(
      adapter.request_device(
        &wgpu::DeviceDescriptor {
          label: Some("HeadlessDevice"),
          required_features: wgpu::Features::empty(),
          required_limits: wgpu::Limits::default(),
        },
        None,
      ),
    )
    .expect("Failed to create device");

    let texture = device.create_texture(&wgpu::TextureDescriptor {
      label: Some("CaptureTexture"),
      size: wgpu::Extent3d {
        width,
        height,
        depth_or_array_layers: 1,
      },
      mip_level_count: 1,
      sample_count: 1,
      dimension: wgpu::TextureDimension::D2,
      format: CAPTURE_FORMAT,
      usage: wgpu::TextureUsages::RENDER_ATTACHMENT
        | wgpu::TextureUsages::COPY_SRC,
      view_formats: &[],
    });

    let padded_row = padded_bytes_per_row(width);
    let staging_buffer =
      device.create_buffer(&wgpu::BufferDescriptor {
        label: Some("StagingBuffer"),
        size: (padded_row * height) as u64,
        usage: wgpu::BufferUsages::MAP_READ
          | wgpu::BufferUsages::COPY_DST,
        mapped_at_creation: false,
      });

    let prim =
      PrimitiveRenderer::new(&device, CAPTURE_FORMAT, width, height);

    let font_data = load_default_font();
    let text = TextRenderer::new(
      &device,
      CAPTURE_FORMAT,
      width,
      height,
      &font_data,
    );

    Self {
      device,
      queue,
      texture,
      staging_buffer,
      prim,
      text,
      width,
      height,
    }
  }

  /// Returns a mutable reference to the primitive renderer.
  pub fn prim(&mut self) -> &mut PrimitiveRenderer {
    &mut self.prim
  }

  /// Returns a mutable reference to the text renderer.
  pub fn text(&mut self) -> &mut TextRenderer {
    &mut self.text
  }

  /// Returns a reference to the wgpu device.
  pub fn device(&self) -> &wgpu::Device {
    &self.device
  }

  /// Returns a reference to the wgpu queue.
  pub fn queue(&self) -> &wgpu::Queue {
    &self.queue
  }

  /// Returns the capture width in pixels.
  pub fn width(&self) -> u32 {
    self.width
  }

  /// Returns the capture height in pixels.
  pub fn height(&self) -> u32 {
    self.height
  }

  /// Renders the current primitive and text state to the off-screen
  /// texture, copies to the staging buffer, maps it, and returns
  /// tightly-packed RGBA pixel data (width * height * 4 bytes).
  pub fn capture(&mut self) -> Vec<u8> {
    // Prepare GPU data.
    self.prim.prepare(&self.device, &self.queue);
    self.text.prepare(&self.device, &self.queue);

    let view = self
      .texture
      .create_view(&wgpu::TextureViewDescriptor::default());

    let mut encoder = self.device.create_command_encoder(
      &wgpu::CommandEncoderDescriptor {
        label: Some("CaptureEncoder"),
      },
    );

    // Render pass: clear to white, draw primitives then text.
    {
      let mut rp =
        encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
          label: Some("CapturePass"),
          color_attachments: &[Some(
            wgpu::RenderPassColorAttachment {
              view: &view,
              resolve_target: None,
              ops: wgpu::Operations {
                load: wgpu::LoadOp::Clear(wgpu::Color {
                  r: 1.0,
                  g: 1.0,
                  b: 1.0,
                  a: 1.0,
                }),
                store: wgpu::StoreOp::Store,
              },
            },
          )],
          depth_stencil_attachment: None,
          ..Default::default()
        });

      self.prim.render(&mut rp);
      self.text.render(&mut rp);
    }

    // Copy texture to staging buffer.
    let padded_row = padded_bytes_per_row(self.width);
    encoder.copy_texture_to_buffer(
      wgpu::ImageCopyTexture {
        texture: &self.texture,
        mip_level: 0,
        origin: wgpu::Origin3d::ZERO,
        aspect: wgpu::TextureAspect::All,
      },
      wgpu::ImageCopyBuffer {
        buffer: &self.staging_buffer,
        layout: wgpu::ImageDataLayout {
          offset: 0,
          bytes_per_row: Some(padded_row),
          rows_per_image: Some(self.height),
        },
      },
      wgpu::Extent3d {
        width: self.width,
        height: self.height,
        depth_or_array_layers: 1,
      },
    );

    self.queue.submit(std::iter::once(encoder.finish()));

    // Map and read back.
    let buffer_slice = self.staging_buffer.slice(..);
    let (tx, rx) = std::sync::mpsc::channel();
    buffer_slice.map_async(wgpu::MapMode::Read, move |result| {
      tx.send(result).unwrap();
    });
    self.device.poll(wgpu::Maintain::Wait);
    rx.recv()
      .expect("GPU channel closed")
      .expect("Failed to map staging buffer");

    let data = buffer_slice.get_mapped_range();
    let unpadded_row = (self.width * 4) as usize;
    let padded_row = padded_row as usize;

    // Strip padding to produce tightly-packed pixels.
    let mut pixels =
      Vec::with_capacity(unpadded_row * self.height as usize);
    for row in 0..self.height as usize {
      let start = row * padded_row;
      pixels
        .extend_from_slice(&data[start..start + unpadded_row]);
    }

    drop(data);
    self.staging_buffer.unmap();

    pixels
  }

  /// Captures the current frame and saves it as a PNG file.
  pub fn save_png<P: AsRef<Path>>(&mut self, path: P) {
    let pixels = self.capture();
    image::save_buffer(
      path,
      &pixels,
      self.width,
      self.height,
      image::ColorType::Rgba8,
    )
    .expect("Failed to save PNG");
  }
}

/// Combines a [`HeadlessRenderer`] with a [`PlotBackend`] for
/// end-to-end plot rendering and capture.
pub struct PlotCapture {
  headless: HeadlessRenderer,
  plot_backend: PlotBackend,
}

impl PlotCapture {
  /// Creates a new plot capture context with the given dimensions.
  pub fn new(width: u32, height: u32) -> Self {
    let headless = HeadlessRenderer::new(width, height);
    let plot_backend = PlotBackend::new(width, height);
    Self {
      headless,
      plot_backend,
    }
  }

  /// Returns the matplot++ figure for configuring plots.
  pub fn figure(&self) -> crate::plotting::Figure {
    self.plot_backend.figure()
  }

  /// Returns the capture width.
  pub fn width(&self) -> u32 {
    self.headless.width()
  }

  /// Returns the capture height.
  pub fn height(&self) -> u32 {
    self.headless.height()
  }

  /// Clears renderers, runs the matplot++ render pipeline through
  /// the FFI callbacks, then captures the result as RGBA pixels.
  pub fn render_and_capture(&mut self) -> Vec<u8> {
    self.headless.prim.clear();
    self.headless.text.clear();
    self.plot_backend.render(
      &mut self.headless.prim,
      &mut self.headless.text,
      None,
    );
    self.headless.capture()
  }

  /// Renders and saves the result as a PNG file.
  pub fn save_png<P: AsRef<Path>>(&mut self, path: P) {
    let pixels = self.render_and_capture();
    image::save_buffer(
      path,
      &pixels,
      self.headless.width,
      self.headless.height,
      image::ColorType::Rgba8,
    )
    .expect("Failed to save PNG");
  }
}
