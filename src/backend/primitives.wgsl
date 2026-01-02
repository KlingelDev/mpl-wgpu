// SDF Primitives Shader

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,    
    @location(1) dim: vec2<f32>,   
    @location(2) color: vec4<f32>,
    @location(3) params: vec4<f32>, // radius, type, line_len, stroke_width
    @location(4) dash_params: vec3<f32>, // dash_len, gap_len, dash_offset
};

struct GlobalUniforms {
    view_proj: mat4x4<f32>,
    screen_size: vec2<f32>,
    _padding: vec2<f32>,
    camera_pos: vec3<f32>,
};
@group(0) @binding(0) var<uniform> globals: GlobalUniforms;
@group(0) @binding(1) var font_tex: texture_2d<f32>;
@group(0) @binding(2) var font_sampler: sampler;

@vertex
fn vs_main(
    @builtin(vertex_index) vertex_index: u32,
    @location(0) pos_a_radius: vec4<f32>,
    @location(1) pos_b_stroke: vec4<f32>,
    @location(2) color: vec4<f32>,
    @location(3) params: vec4<f32>,
    @location(4) pos_c_pad: vec4<f32>,
) -> VertexOutput {
    var out: VertexOutput;
    
    let pos_a = pos_a_radius.xyz;
    let radius = pos_a_radius.w;
    let pos_b = pos_b_stroke.xyz;
    let stroke_width = pos_b_stroke.w;
    let pos_c = pos_c_pad.xyz;

    let prim_type = u32(params.x + 0.5);
    let dash_len = params.y;
    let gap_len = params.z;
    let dash_offset = params.w;

    let idx = vertex_index % 6u;
    
    // Check if prim_type is Triangle (30)
    if (prim_type == 30u) {
        // Fix: Only draw ONE triangle (indices 0,1,2).
        if (idx > 2u) {
            out.position = vec4<f32>(-3.0, -3.0, 0.0, 1.0); // Degenerate
            return out;
        }

        var p = pos_a;
        if (idx == 1u) { p = pos_b; }
        if (idx == 2u) { p = pos_c; }

        out.position = globals.view_proj * vec4<f32>(p, 1.0);
        out.uv = vec2<f32>(0.0, 0.0);
        out.dim = vec2<f32>(0.0, 0.0);
        out.color = color;
        out.params = vec4<f32>(0.0, f32(prim_type), 0.0, 0.0);
        out.dash_params = p; // World Pos for normals
        return out;
    }

    var x = -0.5; var y = -0.5;
    if (idx == 1u || idx == 2u || idx == 4u) { x = 0.5; }
    if (idx == 2u || idx == 4u || idx == 5u) { y = 0.5; }
    
    var center: vec3<f32>;
    var size: vec2<f32>;
    var rotation = mat3x3<f32>(
        vec3<f32>(1.0, 0.0, 0.0),
        vec3<f32>(0.0, 1.0, 0.0),
        vec3<f32>(0.0, 0.0, 1.0)
    );

    var line_len = 0.0;

    // Check if view_proj is mostly identity (2D pixel coordinates)
    // FORCE 2D mode for now because we are using an Ortho matrix which falsifies the identity check
    let is_2d = true; 
    
    // ... (lines 86-90) ...

    if (prim_type == 2u) { // Line
        let delta = pos_b - pos_a;
        line_len = length(delta);
        let dir = normalize(delta + vec3<f32>(1e-9, 1e-9, 1e-9)); // Safety
        
        if (is_2d) {
            // 2D Line Expansion: Use perpendicular in XY plane
            let perp = vec3<f32>(-dir.y, dir.x, 0.0);
            // Store direction and perp for manual expansion later
            out.dash_params = vec3<f32>(dir.x, dir.y, perp.x);
        } else {
            // 3D Billboarding logic
             var view_dir = globals.camera_pos - (pos_a + delta * 0.5);
            if (length(view_dir) < 1e-6) { view_dir = vec3<f32>(0.0, 0.0, 1.0); }
            view_dir = normalize(view_dir);
            
            var perp = cross(dir, view_dir);
            if (length(perp) < 1e-6) {
                if (abs(dir.z) < 0.9) { perp = cross(dir, vec3<f32>(0.0, 0.0, 1.0)); }
                else { perp = cross(dir, vec3<f32>(0.0, 1.0, 0.0)); }
            }
            perp = normalize(perp);
            let actual_up = cross(perp, dir);
            rotation = mat3x3<f32>(dir, perp, actual_up);
        }
        
        center = pos_a + delta * 0.5;
        size = vec2<f32>(line_len + radius * 4.0, radius * 2.0);
    } else if (prim_type == 0u) { // Rect
        center = pos_a;
        size = pos_b.xy;
    } else if (prim_type == 100u) { // Text Glyph
        // Text uses center + width/height in pos_b.xy
        center = pos_a;
        size = pos_b.xy;  // Width and height are passed in pos_b
    } else { // Circles/Markers
        center = pos_a;
        size = vec2<f32>(radius * 2.0, radius * 2.0);
        if (!is_2d) {
             var view_dir = globals.camera_pos - center;
            if (length(view_dir) < 1e-6) { view_dir = vec3<f32>(0.0, 0.0, 1.0); }
            view_dir = normalize(view_dir);
            
            var right = cross(vec3<f32>(0.0, 0.0, 1.0), view_dir);
            if (length(right) < 1e-6) { right = vec3<f32>(1.0, 0.0, 0.0); }
            right = normalize(right);
            
            let up = cross(view_dir, right);
            rotation = mat3x3<f32>(right, up, view_dir);
        }
    }

    var effective_radius = radius;
    var expansion = radius * 4.0 + 0.02;
    if (expansion < 0.04) { expansion = 0.04; }
    
    let viz_size = size + vec2<f32>(expansion, expansion);
    let local_pos = vec3<f32>(x * viz_size.x, y * viz_size.y, 0.0);
    
    var world_pos: vec3<f32>;
    
    if (is_2d && prim_type == 2u) {
        let dir_x = out.dash_params.x;
        let dir_y = out.dash_params.y;
        let dir_vec = vec3<f32>(dir_x, dir_y, 0.0);
        let perp_vec = vec3<f32>(-dir_y, dir_x, 0.0);
        world_pos = center + dir_vec * local_pos.x + perp_vec * local_pos.y;
    } else {
        world_pos = center + rotation * local_pos;
    }

    if (is_2d) {
        // Use the Matrix if available (Ortho), otherwise manual
        out.position = globals.view_proj * vec4<f32>(world_pos, 1.0);
    } else {
        out.position = globals.view_proj * vec4<f32>(world_pos, 1.0);
    }

    out.uv = local_pos.xy;
    
    // Text UV Mapping (pos_c_pad = u0, v0, u1, v1)
    if (prim_type == 100u) {
        let u0 = pos_c_pad.x;
        let v0 = pos_c_pad.y;
        let u1 = pos_c_pad.z;
        let v1 = pos_c_pad.w;
        var u = u0; // Left (x < 0)
        var v = v0; // Top (y < 0)
        if (x > 0.0) { u = u1; }
        if (y > 0.0) { v = v1; }
        out.uv = vec2<f32>(u, v);
    }
    out.dim = size * 0.5;
    if (!is_2d) { out.dim = vec2<f32>(out.dim.x, effective_radius); }
    out.color = color;
    out.params = vec4<f32>(radius, f32(prim_type), line_len, stroke_width);
    out.dash_params = vec3<f32>(dash_len, gap_len, dash_offset);
    
    return out;
}

fn sd_rounded_box(p: vec2<f32>, b: vec2<f32>, r: f32) -> f32 {
    let q = abs(p) - b + r;
    return length(max(q, vec2<f32>(0.0))) + min(max(q.x, q.y), 0.0) - r;
}

fn sd_capsule(p: vec2<f32>, half_length: f32, radius: f32) -> f32 {
    let px = abs(p.x) - half_length;
    return length(vec2<f32>(max(px, 0.0), p.y)) - radius;
}

fn sd_circle(p: vec2<f32>, r: f32) -> f32 {
    return length(p) - r;
}

fn sd_plus(p: vec2<f32>, r: f32) -> f32 {
    let q = abs(p);
    let d1 = max(q.x - r, q.y - r * 0.2);
    let d2 = max(q.x - r * 0.2, q.y - r);
    return min(d1, d2);
}

fn sd_cross(p: vec2<f32>, r: f32) -> f32 {
    let q = abs(mat2x2<f32>(0.707, 0.707, -0.707, 0.707) * p);
    let d1 = max(q.x - r, q.y - r * 0.2);
    let d2 = max(q.x - r * 0.2, q.y - r);
    return min(d1, d2);
}

fn sd_star(p: vec2<f32>, r: f32) -> f32 {
    let k1 = vec2<f32>(0.809, -0.587);
    let k2 = vec2<f32>(-0.809, -0.587);
    var px = abs(p.x);
    var py = p.y;
    px -= 2.0 * min(dot(k1, vec2<f32>(px, py)), 0.0) * k1.x;
    py -= 2.0 * min(dot(k1, vec2<f32>(px, py)), 0.0) * k1.y;
    px -= 2.0 * min(dot(k2, vec2<f32>(px, py)), 0.0) * k2.x;
    py -= 2.0 * min(dot(k2, vec2<f32>(px, py)), 0.0) * k2.y;
    px -= clamp(px, r * 0.38, r * 0.809);
    py -= r;
    return length(vec2<f32>(px, py)) * sign(py);
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {
    var dist = 0.0;
    let prim_type = u32(in.params.y + 0.5);
    let radius = in.params.x;
    let stroke_width = in.params.w;
    
    if (prim_type == 100u) { // Text
        let alpha = textureSample(font_tex, font_sampler, in.uv).r;
        if (alpha < 0.01) { discard; }
        return vec4<f32>(in.color.rgb, in.color.a * alpha);
    } else if (prim_type == 0u) { // Rect
        dist = sd_rounded_box(in.uv, in.dim, radius);
    } else if (prim_type == 1u) { // Circle
        let d = sd_circle(in.uv, in.dim.x);
        if (stroke_width > 0.0) {
            // Stroked (Ring)
            dist = abs(d) - stroke_width * 0.5;
        } else {
            // Filled
            dist = d;
        }
    } else if (prim_type == 2u) { // Line / Capsule
        let half_length = in.params.z * 0.5;
        dist = sd_capsule(in.uv, half_length, radius);
    } else if (prim_type == 10u) { // Plus
        dist = sd_plus(in.uv, in.dim.x);
    } else if (prim_type == 11u) { // Cross
        dist = sd_cross(in.uv, in.dim.x);
    } else if (prim_type == 12u) { // Star
        dist = sd_star(in.uv, in.dim.x);
    } else if (prim_type == 13u) { // Diamond
        let q = abs(mat2x2<f32>(0.707, 0.707, -0.707, 0.707) * in.uv);
        dist = max(q.x, q.y) - in.dim.x * 0.707;
    } else if (prim_type == 30u) { // Triangle (solid fill)
        let pos = in.dash_params;
        let dx = dpdx(pos);
        let dy = dpdy(pos);
        let N = normalize(cross(dx, dy));
        let L = normalize(vec3<f32>(1.0, 1.0, 1.0));
        let ambient = 0.4;
        let diffuse = max(dot(N, L), 0.0);
        let lighting = ambient + (1.0 - ambient) * diffuse;
        return vec4<f32>(in.color.rgb * lighting, in.color.a);
    } else if (prim_type == 31u) { // Triangle (unlit)
        return in.color;
    }
    
    // Antialiasing using screen-space derivatives
    let aa_width = fwidth(dist);
    let blur = clamp(aa_width, 0.5, 2.0);
    let alpha = 1.0 - smoothstep(-blur, blur, dist);
    
    if (alpha <= 0.001) { discard; }
    
    var final_color = in.color;
    final_color.a *= alpha;
    
    return final_color;
}
