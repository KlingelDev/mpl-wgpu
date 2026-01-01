// Copyright (c) 2026 Karl Ruskowski
// SPDX-License-Identifier: MIT

//! Simple Rust example with window using winit + wgpu

use mpl_wgpu::Figure;
use winit::{
    event::{Event, WindowEvent},
    event_loop::{ControlFlow, EventLoop},
    window::WindowBuilder,
};

fn main() -> anyhow::Result<()> {
    env_logger::init();
    println!("mpl-wgpu Simple Plot Example (Rust)");

    // Create event loop and window
    let event_loop = EventLoop::new()?;
    let window = WindowBuilder::new()
        .with_title("mpl-wgpu Example (Rust)")
        .with_inner_size(winit::dpi::PhysicalSize::new(800, 600))
        .build(&event_loop)?;

    // Create wgpu instance and surface
    let instance = wgpu::Instance::new(wgpu::InstanceDescriptor {
        backends: wgpu::Backends::all(),
        ..Default::default()
    });

    let surface = instance.create_surface(&window)?;
    
    // Request adapter and device
    let adapter = pollster::block_on(instance.request_adapter(
        &wgpu::RequestAdapterOptions {
            power_preference: wgpu::PowerPreference::default(),
            compatible_surface: Some(&surface),
            force_fallback_adapter: false,
        },
    ))
    .ok_or_else(|| anyhow::anyhow!("Failed to get adapter"))?;

    let (device, queue) = pollster::block_on(adapter.request_device(
        &wgpu::DeviceDescriptor {
            label: Some("Main Device"),
            required_features: wgpu::Features::empty(),
            required_limits: wgpu::Limits::default(),
        },
        None,
    ))?;

    // Configure surface
    let size = window.inner_size();
    let mut config = surface
        .get_default_config(&adapter, size.width, size.height)
        .ok_or_else(|| anyhow::anyhow!("Surface incompatible"))?;
    surface.configure(&device, &config);

    println!("wgpu initialized: {}x{}", size.width, size.height);

    // TODO: Create figure and plot
    // let mut fig = Figure::new();
    // let x: Vec<f64> = (0..20).map(|i| i as f64).collect();
    // let y: Vec<f64> = x.iter().map(|&x| x * x).collect();
    // fig.plot(&x, &y)?;

    println!("Window open, press ESC or close to exit");

    // Main event loop
    event_loop.run(move |event, elwt| {
        elwt.set_control_flow(ControlFlow::Wait);

        match event {
            Event::WindowEvent { event, .. } => match event {
                WindowEvent::CloseRequested => elwt.exit(),
                WindowEvent::KeyboardInput { event, .. } => {
                    if event.physical_key
                        == winit::keyboard::PhysicalKey::Code(
                            winit::keyboard::KeyCode::Escape,
                        )
                    {
                        elwt.exit();
                    }
                }
                WindowEvent::Resized(new_size) => {
                    config.width = new_size.width;
                    config.height = new_size.height;
                    surface.configure(&device, &config);
                }
                WindowEvent::RedrawRequested => {
                    // Get current texture
                    let frame = surface
                        .get_current_texture()
                        .expect("Failed to get surface texture");
                    
                    let view =
                        frame.texture.create_view(
                            &wgpu::TextureViewDescriptor::default()
                        );

                    // Create command encoder
                    let mut encoder = device.create_command_encoder(
                        &wgpu::CommandEncoderDescriptor {
                            label: Some("Render Encoder"),
                        },
                    );

                    // Clear screen
                    {
                        let _render_pass = encoder.begin_render_pass(
                            &wgpu::RenderPassDescriptor {
                                label: Some("Clear Pass"),
                                color_attachments: &[Some(
                                    wgpu::RenderPassColorAttachment {
                                        view: &view,
                                        resolve_target: None,
                                        ops: wgpu::Operations {
                                            load: wgpu::LoadOp::Clear(
                                                wgpu::Color {
                                                    r: 0.93,
                                                    g: 0.93,
                                                    b: 0.93,
                                                    a: 1.0,
                                                },
                                            ),
                                            store: wgpu::StoreOp::Store,
                                        },
                                    },
                                )],
                               depth_stencil_attachment: None,
                                ..Default::default()
                            },
                        );
                    }

                    // TODO: Render plot here

                    queue.submit(std::iter::once(encoder.finish()));
                    frame.present();
                }
                _ => {}
            },
            Event::AboutToWait => {
                window.request_redraw();
            }
            _ => {}
        }
    })?;

    Ok(())
}
