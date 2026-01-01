// SDF Primitives Shader

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,    
    @location(1) dim: vec2<f32>,   
    @location(2) color: vec4<f32>,
    @location(3) @interpolate(flat) params: vec4<f32>, // radius, type, line_len, stroke_width
    @location(4) dash_params: vec3<f32>, // dash_len, gap_len, dash_offset
};

struct GlobalUniforms {
    view_proj: mat4x4<f32>,
    screen_size: vec2<f32>,
    _padding: vec2<f32>,
    camera_pos: vec3<f32>,
};
@group(0) @binding(0) var<uniform> globals: GlobalUniforms;

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
    

    
    // Check if prim_type works
    if (prim_type == 30u) {
        // ... // Triangle
        // Fix: Only draw ONE triangle (indices 0,1,2).
        // Emitting 3,4,5 as duplicate would cause double-blending/Z-fighting.
        if (idx > 2u) {
            // Degenerate/Invisible: Move off-screen
            out.position = vec4<f32>(-3.0, -3.0, 0.0, 1.0);
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
        // Pass World Position in dash_params for Fragment Shader normal calculation
        out.dash_params = p;
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
    let is_2d = abs(globals.view_proj[0][0] - 1.0) < 0.0001 && 
                abs(globals.view_proj[1][1] - 1.0) < 0.0001 &&
                abs(globals.view_proj[3][3] - 1.0) < 0.0001 &&
                abs(globals.view_proj[0][1]) < 0.0001 &&
                abs(globals.view_proj[1][0]) < 0.0001;

    if (prim_type == 2u) { // Line
        let delta = pos_b - pos_a;
        line_len = length(delta);
        let dir = normalize(delta + vec3<f32>(1e-9, 1e-9, 1e-9)); // Safety
        
        if (is_2d) {
            // Store dir for manual rotation (matrix mult doesn't work correctly)
            let perp = vec3<f32>(-dir.y, dir.x, 0.0);
            // Store dir in line_dir for later manual rotation
            out.dash_params = vec3<f32>(dir.x, dir.y, perp.x);  // dir.x, dir.y, perp.x
        } else {
            // 3D Billboarding for lines
            var view_dir = globals.camera_pos - (pos_a + delta * 0.5);
            if (length(view_dir) < 1e-6) { view_dir = vec3<f32>(0.0, 0.0, 1.0); }
            view_dir = normalize(view_dir);
            
            var perp = cross(dir, view_dir);
            if (length(perp) < 1e-6) {
                // View is parallel to line, pick any perpendicular
                if (abs(dir.z) < 0.9) { perp = cross(dir, vec3<f32>(0.0, 0.0, 1.0)); }
                else { perp = cross(dir, vec3<f32>(0.0, 1.0, 0.0)); }
            }
            perp = normalize(perp);
            let actual_up = cross(perp, dir);
            rotation = mat3x3<f32>(dir, perp, actual_up);
        }
        
        center = pos_a + delta * 0.5;
        // Add extra length (radius*4 instead of radius*2) to ensure segments overlap
        size = vec2<f32>(line_len + radius * 4.0, radius * 2.0);
    } else if (prim_type == 0u) { // Rect
        center = pos_a;
        size = pos_b.xy;
    } else { // Circles/Markers
        center = pos_a;
        size = vec2<f32>(radius * 2.0, radius * 2.0);
        if (!is_2d) {
            // 3D Billboarding for points/circles
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
    // Use large fixed expansion (4x radius) + fixed padding for Blur
    var expansion = radius * 4.0 + 0.02;
    
    // Ensure minimum expansion for very thin lines
    if (expansion < 0.04) { expansion = 0.04; }
    
    let viz_size = size + vec2<f32>(expansion, expansion);
    let local_pos = vec3<f32>(x * viz_size.x, y * viz_size.y, 0.0);
    
    // Compute world position
    var world_pos: vec3<f32>;
    
    if (is_2d && prim_type == 2u) {
        // Manual rotation for 2D lines
        let dir_x = out.dash_params.x;
        let dir_y = out.dash_params.y;
        let dir_vec = vec3<f32>(dir_x, dir_y, 0.0);
        let perp_vec = vec3<f32>(-dir_y, dir_x, 0.0);
        world_pos = center + dir_vec * local_pos.x + perp_vec * local_pos.y;
    } else {
        world_pos = center + rotation * local_pos;
        
        // Manual offset removed
        // if (!is_2d && prim_type == 2u) { ... }
    }

    if (is_2d) {
        let ndc = (world_pos.xy / globals.screen_size) * 2.0 - 1.0;
        out.position = vec4<f32>(ndc.x, -ndc.y, world_pos.z, 1.0);
    } else {
        out.position = globals.view_proj * vec4<f32>(world_pos, 1.0);
    }

    out.uv = local_pos.xy;
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

// Proper capsule SDF: horizontal capsule with half_length and radius
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
    
    if (prim_type == 0u) { // Rect
        dist = sd_rounded_box(in.uv, in.dim, radius);
    } else if (prim_type == 1u) { // Circle
        dist = abs(sd_circle(in.uv, in.dim.x)) - stroke_width * 0.5;
    } else if (prim_type == 2u) { // Line / Capsule
        // Use capsule SDF with half_length from line_len stored in params.z
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
        // Flat Shading: Calculate Face Normal from position derivatives
        let pos = in.dash_params;
        // Standard derivative trick for flat normals
        let dx = dpdx(pos);
        let dy = dpdy(pos);
        let N = normalize(cross(dx, dy));
        
        // Simple Directional Light from Top-Right-Front
        let L = normalize(vec3<f32>(1.0, 1.0, 1.0));
        
        // Lighting Model: Ambient + Diffuse
        let ambient = 0.4;
        let diffuse = max(dot(N, L), 0.0);
        
        let lighting = ambient + (1.0 - ambient) * diffuse;
        return vec4<f32>(in.color.rgb * lighting, in.color.a);
    } else if (prim_type == 31u) { // Triangle (unlit)
        return in.color;
    }
    
    // Hard edges to eliminate noise/grain
    let blur = 0.0001; 
    let alpha = 1.0 - smoothstep(-blur, 0.0, dist);
    
    // Discard if fully transparent
    if (alpha <= 0.0) { discard; }
    
    // DEBUG: Show SDF distance as color (blue=inside, red=outside)
    // if (prim_type == 2u) {
    //     return vec4<f32>(max(0.0, dist / 10.0), 0.0, max(0.0, -dist / 10.0), 1.0);
    // }
    
    // if (alpha < 0.05) { discard; } // REMOVED to prevent jagged edges
    
    var final_color = in.color;
    final_color.a *= alpha;
    
    return final_color;
}
