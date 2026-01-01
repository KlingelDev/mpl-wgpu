use wgpu_text::glyph_brush::{Section, Text, ab_glyph::FontArc};
use wgpu_text::{BrushBuilder, TextBrush};
use glam::{Vec2, Vec4};

struct QueuedText {
    text: String,
    pos: Vec2,
    size: f32,
    color: Vec4,
}

pub struct TextRenderer {
    brush: TextBrush<FontArc>,
    width: u32,
    height: u32,
    queued_texts: Vec<QueuedText>,
}

impl TextRenderer {
    pub fn new(
        device: &wgpu::Device, 
        format: wgpu::TextureFormat, 
        width: u32, 
        height: u32, 
        font_data: &[u8]
    ) -> Self {
        let font = FontArc::try_from_vec(font_data.to_vec()).expect("Failed to parse font");
        let brush = BrushBuilder::using_font(font)
            .build(device, width, height, format);
        
        Self {
            brush,
            width,
            height,
            queued_texts: Vec::new(),
        }
    }

    pub fn resize(&mut self, queue: &wgpu::Queue, width: u32, height: u32) {
        self.width = width;
        self.height = height;
        self.brush.resize_view(width as f32, height as f32, queue);
    }

    // Queue text for the current frame
    pub fn draw_text(&mut self, text: &str, pos: Vec2, size: f32, color: Vec4) {
        self.queued_texts.push(QueuedText {
            text: text.to_string(),
            pos,
            size,
            color,
        });
    }

    // Process all queued text and upload to GPU
    pub fn prepare(&mut self, device: &wgpu::Device, queue: &wgpu::Queue) {
        let sections: Vec<Section> = self.queued_texts.iter().map(|qt| {
            Section::default()
                .add_text(
                    Text::new(&qt.text)
                        .with_scale(qt.size)
                        .with_color([qt.color.x, qt.color.y, qt.color.z, qt.color.w])
                )
                .with_screen_position((qt.pos.x, qt.pos.y))
        }).collect();

        self.brush.queue(device, queue, sections).unwrap();
        
        // Clear for next frame
        self.queued_texts.clear();
    }

    pub fn clear(&mut self) {
        self.queued_texts.clear();
    }

    pub fn render<'a>(&'a mut self, rpass: &mut wgpu::RenderPass<'a>) {
        self.brush.draw(rpass);
    }

    pub fn measure_text(&self, text: &str, size: f32) -> Vec2 {
        // Approximate for FiraCode (Monospaced)
        // Average width approx 0.5 * height (tighter fit)
        let width = text.len() as f32 * size * 0.5;
        let height = size;
        Vec2::new(width, height)
    }
}
