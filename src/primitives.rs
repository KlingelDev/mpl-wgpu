use glam::{Vec2, Vec3, Vec4};

#[repr(C)]
#[derive(Debug, Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
pub struct Instance {
    pub pos_a_radius: [f32; 4],
    pub pos_b_width: [f32; 4],
    pub color: [f32; 4],
    pub params: [f32; 4], // prim_type, dash_len, gap_len, dash_offset
    pub pos_c_pad: [f32; 4],
}

pub struct PrimitiveRenderer {
    pipeline_depth_write: wgpu::RenderPipeline,
    pipeline_lines: wgpu::RenderPipeline,
    bind_group: wgpu::BindGroup,
    uniform_buffer: wgpu::Buffer,
    instance_buffer: wgpu::Buffer,
    instances: Vec<Instance>,
    screen_size: Vec2,
    view_proj: glam::Mat4,
    capacity: usize,
}

impl PrimitiveRenderer {
    pub fn new(
        device: &wgpu::Device,
        format: wgpu::TextureFormat,
        width: u32,
        height: u32,
    ) -> Self {
        let shader = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("PrimitiveShader"),
            source: wgpu::ShaderSource::Wgsl(include_str!("primitives.wgsl").into()),
        });

        let uniform_buffer = device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("PrimitiveUniforms"),
            size: 64 + 16 + 16, // Mat4 + Vec2 + CameraPos + padding
            usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });

        let bind_group_layout =
            device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
                label: Some("PrimitiveBindGroup"),
                entries: &[wgpu::BindGroupLayoutEntry {
                    binding: 0,
                    visibility: wgpu::ShaderStages::VERTEX_FRAGMENT,
                    ty: wgpu::BindingType::Buffer {
                        ty: wgpu::BufferBindingType::Uniform,
                        has_dynamic_offset: false,
                        min_binding_size: None,
                    },
                    count: None,
                }],
            });

        let bind_group = device.create_bind_group(&wgpu::BindGroupDescriptor {
            layout: &bind_group_layout,
            entries: &[wgpu::BindGroupEntry {
                binding: 0,
                resource: uniform_buffer.as_entire_binding(),
            }],
            label: None,
        });

        let pipeline_layout =
            device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
                bind_group_layouts: &[&bind_group_layout],
                push_constant_ranges: &[],
                label: None,
            });

        let vertex_state = wgpu::VertexState {
            module: &shader,
            entry_point: "vs_main",
            buffers: &[wgpu::VertexBufferLayout {
                array_stride: std::mem::size_of::<Instance>()
                    as wgpu::BufferAddress,
                step_mode: wgpu::VertexStepMode::Instance,
                attributes: &wgpu::vertex_attr_array![
                    0 => Float32x4, // pos_a + radius
                    1 => Float32x4, // pos_b + stroke_width
                    2 => Float32x4, // color
                    3 => Float32x4, // params (type, dash_len, gap_len, offset)
                    4 => Float32x4  // pos_c + padding
                ],
            }],
            compilation_options: wgpu::PipelineCompilationOptions::default(),
        };

        let fragment_state = Some(wgpu::FragmentState {
            module: &shader,
            entry_point: "fs_main",
            targets: &[Some(wgpu::ColorTargetState {
                format,
                blend: Some(wgpu::BlendState::ALPHA_BLENDING),
                write_mask: wgpu::ColorWrites::ALL,
            })],
            compilation_options: wgpu::PipelineCompilationOptions::default(),
        });

        let pipeline_depth_write =
            device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
                label: Some("PrimitivePipeline_DepthWrite"),
                layout: Some(&pipeline_layout),
                vertex: vertex_state.clone(),
                fragment: fragment_state.clone(),
                primitive: wgpu::PrimitiveState {
                    cull_mode: None, // Cull back faces to prevent self-intersection noise
                    front_face: wgpu::FrontFace::Ccw,
                    ..Default::default()
                },
                depth_stencil: None,
                multisample: wgpu::MultisampleState {
                    count: 1, 
                    mask: !0,
                    alpha_to_coverage_enabled: false,
                },
                multiview: None,
            });

        let pipeline_lines =
            device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
                label: Some("PrimitivePipeline_Lines"),
                layout: Some(&pipeline_layout),
                vertex: vertex_state,
                fragment: fragment_state,
                primitive: wgpu::PrimitiveState {
                    cull_mode: None,
                    ..Default::default()
                },
                depth_stencil: None,
                multisample: wgpu::MultisampleState {
                    count: 1, 
                    mask: !0,
                    alpha_to_coverage_enabled: false,
                },
                multiview: None,
            });

        let initial_capacity = 1024;
        let instance_buffer = device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("InstanceBuffer"),
            size: (initial_capacity * std::mem::size_of::<Instance>()) as u64,
            usage: wgpu::BufferUsages::VERTEX | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });

        Self {
            pipeline_depth_write,
            pipeline_lines,
            bind_group,
            uniform_buffer,
            instance_buffer,
            instances: Vec::with_capacity(initial_capacity),
            screen_size: Vec2::new(width as f32, height as f32),
            view_proj: glam::Mat4::IDENTITY,
            capacity: initial_capacity,
        }
    }

    pub fn resize(&mut self, queue: &wgpu::Queue, width: u32, height: u32) {
        self.screen_size = Vec2::new(width as f32, height as f32);
        self.update_uniforms(queue);
    }

    pub fn set_view_projection(&mut self, queue: &wgpu::Queue, matrix: glam::Mat4) {
        self.view_proj = matrix;
        self.update_uniforms(queue);
    }

    pub fn set_camera_pos(&mut self, queue: &wgpu::Queue, pos: glam::Vec3) {
        let mut data = [0.0f32; 16 + 4 + 4];
        data[0..16].copy_from_slice(self.view_proj.as_ref());
        data[16] = self.screen_size.x;
        data[17] = self.screen_size.y;
        data[20] = pos.x;
        data[21] = pos.y;
        data[22] = pos.z;
        queue.write_buffer(&self.uniform_buffer, 0, bytemuck::cast_slice(&data));
    }

    fn update_uniforms(&self, queue: &wgpu::Queue) {
        let mut data = [0.0f32; 16 + 4 + 4];
        data[0..16].copy_from_slice(self.view_proj.as_ref());
        data[16] = self.screen_size.x;
        data[17] = self.screen_size.y;
        // Padding/CameraPos (will be updated by set_camera_pos)
        queue.write_buffer(&self.uniform_buffer, 0, bytemuck::cast_slice(&data));
    }

    pub fn draw_rect(
        &mut self,
        pos: Vec2,
        size: Vec2,
        color: Vec4,
        radius: f32,
        stroke_width: f32,
    ) {
        self.instances.push(Instance {
            pos_a_radius: [pos.x, pos.y, 0.0, radius],
            pos_b_width: [size.x, size.y, 0.0, stroke_width],
            color: [color.x, color.y, color.z, color.w],
            params: [0.0, 0.0, 0.0, 0.0],
            pos_c_pad: [0.0; 4],
        });
    }

    pub fn draw_circle(
        &mut self,
        center: Vec3,
        radius: f32,
        color: Vec4,
        stroke_width: f32,
        marker_type: u32,
    ) {
        self.instances.push(Instance {
            pos_a_radius: [center.x, center.y, center.z, radius],
            pos_b_width: [0.0, 0.0, 0.0, stroke_width],
            color: [color.x, color.y, color.z, color.w],
            params: [marker_type as f32, 0.0, 0.0, 0.0],
            pos_c_pad: [0.0; 4],
        });
    }

    pub fn draw_oval(
        &mut self,
        center: Vec2,
        radii: Vec2,
        color: Vec4,
        stroke_width: f32,
    ) {
        self.instances.push(Instance {
            pos_a_radius: [center.x, center.y, 0.0, radii.x],
            pos_b_width: [radii.y, 0.0, 0.0, stroke_width],
            color: [color.x, color.y, color.z, color.w],
            params: [1.0, 0.0, 0.0, 0.0], // Circle/Oval
            pos_c_pad: [0.0; 4],
        });
    }

    pub fn draw_marker(
        &mut self,
        center: Vec2,
        radii: Vec2,
        marker_type: u32,
        color: Vec4,
        stroke_width: f32,
    ) {
        self.instances.push(Instance {
            pos_a_radius: [center.x, center.y, 0.0, radii.x],
            pos_b_width: [radii.y, 0.0, 0.0, stroke_width],
            color: [color.x, color.y, color.z, color.w],
            params: [(10 + marker_type) as f32, 0.0, 0.0, 0.0],
            pos_c_pad: [0.0; 4],
        });
    }

    pub fn draw_line(
        &mut self,
        start: Vec3,
        end: Vec3,
        thickness: f32,
        color: Vec4,
        dash_len: f32,
        gap_len: f32,
        dash_offset: f32,
    ) {
        self.instances.push(Instance {
            pos_a_radius: [start.x, start.y, start.z, thickness * 0.5],
            pos_b_width: [end.x, end.y, end.z, 0.0],
            color: [color.x, color.y, color.z, color.w],
            params: [2.0, dash_len, gap_len, dash_offset],
            pos_c_pad: [0.0; 4],
        });
    }

    pub fn draw_triangle_unlit(
        &mut self,
        p0: Vec3,
        p1: Vec3,
        p2: Vec3,
        color: Vec4,
    ) {
        self.instances.push(Instance {
            pos_a_radius: [p0.x, p0.y, p0.z, 0.0],
            pos_b_width: [p1.x, p1.y, p1.z, 0.0],
            color: [color.x, color.y, color.z, color.w],
            params: [31.0, 0.0, 0.0, 0.0], // Unlit Triangle
            pos_c_pad: [p2.x, p2.y, p2.z, 0.0],
        });
    }

    pub fn draw_triangle(
        &mut self,
        p0: Vec3,
        p1: Vec3,
        p2: Vec3,
        color: Vec4,
    ) {
        self.instances.push(Instance {
            pos_a_radius: [p0.x, p0.y, p0.z, 0.0],
            pos_b_width: [p1.x, p1.y, p1.z, 0.0],
            color: [color.x, color.y, color.z, color.w],
            params: [30.0, 0.0, 0.0, 0.0], // Triangle
            pos_c_pad: [p2.x, p2.y, p2.z, 0.0],
        });
    }

    pub fn prepare(&mut self, device: &wgpu::Device, queue: &wgpu::Queue) {
        if self.instances.is_empty() {
            return;
        }

        // Sort: Faces (30, 31) first.
        self.instances.sort_by_key(|i| {
            let t = i.params[0] as u32;
            t != 30 && t != 31
        });

        let size = (self.instances.len() * std::mem::size_of::<Instance>()) as u64;
        if size > self.instance_buffer.size() {
            self.instance_buffer = device.create_buffer(&wgpu::BufferDescriptor {
                label: Some("InstanceBuffer"),
                size,
                usage: wgpu::BufferUsages::VERTEX | wgpu::BufferUsages::COPY_DST,
                mapped_at_creation: false,
            });
            self.capacity = self.instances.len();
        }

        queue.write_buffer(
            &self.instance_buffer,
            0,
            bytemuck::cast_slice(&self.instances),
        );
    }

    pub fn render<'a>(&'a self, rp: &mut wgpu::RenderPass<'a>) {
        if self.instances.is_empty() {
            return;
        }

        rp.set_bind_group(0, &self.bind_group, &[]);
        rp.set_vertex_buffer(0, self.instance_buffer.slice(..));

        // Find split point between Faces (type 30, 31) and everything else.
        let split_idx = self.instances.partition_point(|i| {
            let t = i.params[0] as u32;
            t == 30 || t == 31
        });

        if split_idx > 0 {
            rp.set_pipeline(&self.pipeline_depth_write);
            rp.draw(0..6, 0..split_idx as u32);
        }

        if split_idx < self.instances.len() {
            rp.set_pipeline(&self.pipeline_lines);
            rp.draw(
                0..6,
                split_idx as u32..self.instances.len() as u32,
            );
        }
    }

    pub fn clear(&mut self) {
        self.instances.clear();
    }
}
