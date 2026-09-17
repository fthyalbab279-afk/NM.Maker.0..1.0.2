const fs = require("fs");
let html = fs.readFileSync("app/src/main/assets/www/index.html", "utf8");

const injectionMarker = "    // =====================================================================\n    // Register asset identifiers on window for direct GML script references";

if (!html.includes(injectionMarker)) {
    console.error("Injection marker not found!");
    process.exit(1);
}

const gm82SuitePart5 = `    // =====================================================================
    // GM8.2 PROJECT (gm82dx9, gm82drag, gm82alpha & gm82chunk Suite)
    // =====================================================================

    // --- GM82DX9: Shaders & Vertex Buffer Pipeline ---
    let _gm82_shader_counter = 100;
    const _gm82_shaders = {};
    let _gm82_current_pixel_shader = null;
    let _gm82_current_vertex_shader = null;

    window.shader_pixel_create = (source) => {
        const id = _gm82_shader_counter++;
        _gm82_shaders[id] = { id, type: 'pixel', source: String(source), uniforms: {} };
        return id;
    };
    window.shader_pixel_create_buffer = (bufId) => {
        const src = window.buffer_to_string ? window.buffer_to_string(bufId) : "";
        return window.shader_pixel_create(src);
    };
    window.shader_pixel_create_base64 = (b64) => {
        try { return window.shader_pixel_create(atob(b64)); } catch(e) { return window.shader_pixel_create(""); }
    };
    window.shader_vertex_create = (source) => {
        const id = _gm82_shader_counter++;
        _gm82_shaders[id] = { id, type: 'vertex', source: String(source), uniforms: {} };
        return id;
    };
    window.shader_vertex_create_buffer = (bufId) => {
        const src = window.buffer_to_string ? window.buffer_to_string(bufId) : "";
        return window.shader_vertex_create(src);
    };
    window.shader_vertex_create_base64 = (b64) => {
        try { return window.shader_vertex_create(atob(b64)); } catch(e) { return window.shader_vertex_create(""); }
    };
    window.shader_destroy = (id) => { delete _gm82_shaders[id]; };
    window.shader_pixel_destroy = window.shader_destroy;
    window.shader_vertex_destroy = window.shader_destroy;

    window.shader_set = (shaderId) => {
        const sh = _gm82_shaders[shaderId];
        if (sh) {
            if (sh.type === 'vertex') _gm82_current_vertex_shader = sh;
            else _gm82_current_pixel_shader = sh;
        }
    };
    window.shader_reset = () => {
        _gm82_current_pixel_shader = null;
        _gm82_current_vertex_shader = null;
    };
    window.shader_pixel_set = (id) => window.shader_set(id);
    window.shader_vertex_set = (id) => window.shader_set(id);
    window.shader_pixel_reset = () => { _gm82_current_pixel_shader = null; };
    window.shader_vertex_reset = () => { _gm82_current_vertex_shader = null; };
    window.shader_pixel_set_passthrough = () => { _gm82_current_pixel_shader = null; };
    window.shader_vertex_set_passthrough = () => { _gm82_current_vertex_shader = null; };

    window.shader_pixel_uniform_exists = (id, name) => true;
    window.shader_vertex_uniform_exists = (id, name) => true;
    window.shader_pixel_uniform_get_address = (id, name) => String(name);
    window.shader_vertex_uniform_get_address = (id, name) => String(name);

    window.shader_pixel_uniform_f = (id, uName, ...vals) => {
        if (_gm82_shaders[id]) _gm82_shaders[id].uniforms[uName] = vals;
    };
    window.shader_vertex_uniform_f = (id, uName, ...vals) => {
        if (_gm82_shaders[id]) _gm82_shaders[id].uniforms[uName] = vals;
    };
    window.shader_pixel_uniform_i = window.shader_pixel_uniform_f;
    window.shader_vertex_uniform_i = window.shader_vertex_uniform_f;
    window.shader_pixel_uniform_b = window.shader_pixel_uniform_f;
    window.shader_vertex_uniform_b = window.shader_vertex_uniform_f;
    window.shader_pixel_uniform_color = (id, uName, col, alpha = 1) => {
        const r = (col & 0xFF) / 255;
        const g = ((col >> 8) & 0xFF) / 255;
        const b = ((col >> 16) & 0xFF) / 255;
        window.shader_pixel_uniform_f(id, uName, r, g, b, alpha);
    };
    window.shader_vertex_uniform_color = window.shader_pixel_uniform_color;
    window.shader_pixel_uniform_matrix = (id, uName, mat) => {
        if (_gm82_shaders[id]) _gm82_shaders[id].uniforms[uName] = mat;
    };
    window.shader_vertex_uniform_matrix = window.shader_pixel_uniform_matrix;
    window.shader_pixel_uniform_f_buffer = (id, uName, bufId, count) => {};
    window.shader_vertex_uniform_f_buffer = (id, uName, bufId, count) => {};
    window.shader_draw_shadertoy = (time) => {};

    // Texture stages & Interpolation
    window.texture_set_stage = (stage, tex) => {};
    window.texture_set_stage_ext = (stage, tex) => {};
    window.texture_set_stage_interpolation = (stage, linear) => {};
    window.texture_set_stage_repeat = (stage, repeat) => {};
    window.texture_set_stage_vertex = (stage, tex) => {};
    window.texture_set_stage_vertex_ext = (stage, tex) => {};
    window.texture_set_stage_vertex_interpolation = (stage, linear) => {};
    window.texture_set_stage_vertex_repeat = (stage, repeat) => {};
    window.texture_set_repeat_ext = (stage, repeat) => {};

    // Vertex Buffer Architecture
    let _gm82_vbuf_counter = 100;
    const _gm82_vbufs = {};
    window.vertex_buffer_create = () => {
        const id = _gm82_vbuf_counter++;
        _gm82_vbufs[id] = { id, vertices: [], format: null };
        return id;
    };
    window.index_buffer_create = () => window.vertex_buffer_create();
    window.vertex_format_create_simple = () => ({ type: 'simple' });
    window.vertex_format_create_default = () => ({ type: 'default' });
    window.vertex_format_add_position = (fmt) => {};
    window.vertex_format_add_texcoord = (fmt) => {};
    window.vertex_format_add_color = (fmt) => {};
    window.vertex_format_add_normal = (fmt) => {};
    window.vertex_buffer_draw = (vbufId) => {};
    window.vertex_instance_set = (instId) => {};

    // 2D Quads & Triangles
    window.draw_tri = (x1, y1, x2, y2, x3, y3, outline = false) => {
        if (!_drawCtx) return;
        _drawCtx.save();
        _drawCtx.fillStyle = _drawColor;
        _drawCtx.strokeStyle = _drawColor;
        _drawCtx.globalAlpha = _drawAlpha;
        _drawCtx.beginPath();
        _drawCtx.moveTo(x1, y1);
        _drawCtx.lineTo(x2, y2);
        _drawCtx.lineTo(x3, y3);
        _drawCtx.closePath();
        if (outline) _drawCtx.stroke(); else _drawCtx.fill();
        _drawCtx.restore();
    };
    window.draw_quad = (x1, y1, x2, y2, x3, y3, x4, y4, outline = false) => {
        if (!_drawCtx) return;
        _drawCtx.save();
        _drawCtx.fillStyle = _drawColor;
        _drawCtx.strokeStyle = _drawColor;
        _drawCtx.globalAlpha = _drawAlpha;
        _drawCtx.beginPath();
        _drawCtx.moveTo(x1, y1);
        _drawCtx.lineTo(x2, y2);
        _drawCtx.lineTo(x3, y3);
        _drawCtx.lineTo(x4, y4);
        _drawCtx.closePath();
        if (outline) _drawCtx.stroke(); else _drawCtx.fill();
        _drawCtx.restore();
    };
    window.draw_quad_color = (x1, y1, x2, y2, x3, y3, x4, y4, c1, c2, c3, c4) => {
        window.draw_quad(x1, y1, x2, y2, x3, y3, x4, y4, false);
    };

    // Application Surface Extras (gm82dx9)
    window.application_surface_enable = (enable) => {};
    window.application_surface_disable = () => {};
    window.application_surface_is_enabled = () => true;
    window.application_surface_resize = (w, h) => {
        if (window.application_surface && window.surface_exists(window.application_surface)) {
            window.surface_free(window.application_surface);
        }
        window.application_surface = window.surface_create(w, h);
    };
    window.application_surface_get_width = () => window.surface_get_width(window.application_surface);
    window.application_surface_get_height = () => window.surface_get_height(window.application_surface);

    // D3D Model Bundle Extensions
    let _gm82_bundle_counter = 100;
    const _gm82_bundles = {};
    window.d3d_model_bundle_create = () => {
        const id = _gm82_bundle_counter++;
        _gm82_bundles[id] = [];
        return id;
    };
    window.d3d_model_bundle_add = (bundleId, modelId) => {
        if (_gm82_bundles[bundleId]) _gm82_bundles[bundleId].push(modelId);
    };
    window.d3d_model_bundle_destroy = (bundleId) => { delete _gm82_bundles[bundleId]; };
    window.d3d_model_bundle_draw = (bundleId, tex) => {
        const models = _gm82_bundles[bundleId];
        if (models && window.d3d_model_draw) {
            models.forEach(m => window.d3d_model_draw(m, 0, 0, 0, tex));
        }
    };
    window.d3d_model_save_g3z = (modelId, filename) => 1;
    window.d3d_model_load_g3z = (modelId, filename) => 1;
    window.d3d_model_create_and_load = (filename) => {
        const m = window.d3d_model_create();
        window.d3d_model_load_g3z(m, filename);
        return m;
    };
    window.d3d_model_bake = (modelId) => {};
    window.d3d_draw_ceiling = (x1, y1, z1, x2, y2, z2, tex, hrep = 1, vrep = 1) => {};
    window.d3d_set_alphablend = (enable) => {};
    window.draw_make_opaque = () => {};

    // --- GM82Drag: File Drag and Drop Support ---
    const _gm82_dragged_files = [];
    window.file_drag_enable = (enable) => {
        if (typeof window !== 'undefined' && window.addEventListener) {
            window.addEventListener('dragover', (e) => e.preventDefault());
            window.addEventListener('drop', (e) => {
                e.preventDefault();
                _gm82_dragged_files.length = 0;
                if (e.dataTransfer && e.dataTransfer.files) {
                    for (let i = 0; i < e.dataTransfer.files.length; i++) {
                        _gm82_dragged_files.push(e.dataTransfer.files[i].name);
                    }
                }
            });
        }
    };
    window.file_drag_count = () => _gm82_dragged_files.length;
    window.file_drag_name = (idx) => _gm82_dragged_files[idx] || "";

    // --- GM82Alpha: Window Transparency & Chromakey ---
    window.window_set_alphablend = (enable, alpha = 1.0) => {
        const c = document.getElementById('gameCanvas');
        if (c) c.style.opacity = String(Math.max(0, Math.min(1, Number(alpha) || 1)));
    };
    window.window_set_chromakey = (enable, color = 0) => {};

    // --- GM82Chunk: Dynamic Room Loading ---
    window.chunk_load_room = (roomName) => {
        const rId = window.room_find ? window.room_find(roomName) : -1;
        if (rId !== -1 && window.room_goto) window.room_goto(rId);
    };
    window.chunk_load_chunk = (chunkFile, ox = 0, oy = 0) => 1;
`;

html = html.replace(injectionMarker, gm82SuitePart5 + "\n" + injectionMarker);

// Update prototype prefixes
const targetPrefix = '"joystick_", "sound_set_group", "sound_group_volume"';
const replacementPrefix = '"joystick_", "sound_set_group", "sound_group_volume", "shader_", "vertex_", "texture_set_", "draw_tri", "draw_quad", "file_drag_", "window_set_alpha", "chunk_"';

if (html.includes(targetPrefix)) {
    html = html.replace(targetPrefix, replacementPrefix);
}

fs.writeFileSync("app/src/main/assets/www/index.html", html, "utf8");
console.log("GM82DX9, GM82Drag, GM82Alpha, GM82Chunk injected successfully!");
