const fs = require("fs");
let html = fs.readFileSync("app/src/main/assets/www/index.html", "utf8");

// 1. MP Grid & Paths system (mp_grid_*, mp_linear_path, mp_potential_path)
const pMp = html.indexOf("window.mp_potential_step =");
if (pMp === -1) {
    console.error("mp_potential_step not found!");
    process.exit(1);
}
const pMpEnd = html.indexOf("};", pMp) + 2;

const mpGridCode = `
    // --- GM8.2 / GML Motion Planning Grid System (mp_grid_*) ---
    const _mp_grids = {};
    let _mp_grid_id_counter = 1;
    window.mp_grid_create = (left, top, hcells, vcells, cellwidth, cellheight) => {
        const id = _mp_grid_id_counter++;
        const grid = {
            id,
            left: Number(left) || 0,
            top: Number(top) || 0,
            hcells: Math.max(1, Math.floor(hcells)),
            vcells: Math.max(1, Math.floor(vcells)),
            cellwidth: Math.max(1, Number(cellwidth) || 16),
            cellheight: Math.max(1, Number(cellheight) || 16),
            cells: new Uint8Array(Math.max(1, Math.floor(hcells)) * Math.max(1, Math.floor(vcells)))
        };
        _mp_grids[id] = grid;
        return id;
    };
    window.mp_grid_destroy = (id) => { delete _mp_grids[id]; };
    window.mp_grid_clear_all = (id) => {
        const g = _mp_grids[id];
        if (g) g.cells.fill(0);
    };
    window.mp_grid_clear_cell = (id, h, v) => {
        const g = _mp_grids[id];
        if (g && h >= 0 && h < g.hcells && v >= 0 && v < g.vcells) g.cells[v * g.hcells + h] = 0;
    };
    window.mp_grid_add_cell = (id, h, v) => {
        const g = _mp_grids[id];
        if (g && h >= 0 && h < g.hcells && v >= 0 && v < g.vcells) g.cells[v * g.hcells + h] = 1;
    };
    window.mp_grid_add_rectangle = (id, x1, y1, x2, y2) => {
        const g = _mp_grids[id];
        if (!g) return;
        const minH = Math.max(0, Math.floor((Math.min(x1, x2) - g.left) / g.cellwidth));
        const maxH = Math.min(g.hcells - 1, Math.floor((Math.max(x1, x2) - g.left) / g.cellwidth));
        const minV = Math.max(0, Math.floor((Math.min(y1, y2) - g.top) / g.cellheight));
        const maxV = Math.min(g.vcells - 1, Math.floor((Math.max(y1, y2) - g.top) / g.cellheight));
        for (let v = minV; v <= maxV; v++) {
            for (let h = minH; h <= maxH; h++) {
                g.cells[v * g.hcells + h] = 1;
            }
        }
    };
    window.mp_grid_add_instances = (id, obj, prec) => {
        const g = _mp_grids[id];
        if (!g) return;
        (window.instances || []).forEach(inst => {
            if (inst.dead || inst._deactivated) return;
            if (obj && obj !== "all" && obj !== -1) {
                if (typeof obj === "string" && inst.def?.name !== obj && inst.def?.id !== obj && inst.objectId !== obj) return;
                if (typeof obj === "object" && inst !== obj) return;
            }
            window.mp_grid_add_rectangle(id, inst.x, inst.y, inst.x + (inst.w || g.cellwidth) - 1, inst.y + (inst.h || g.cellheight) - 1);
        });
    };
    window.mp_grid_path = (id, pathId, xstart, ystart, xgoal, ygoal, allowdiag) => {
        const g = _mp_grids[id];
        if (!g) return false;
        const startH = Math.floor((xstart - g.left) / g.cellwidth);
        const startV = Math.floor((ystart - g.top) / g.cellheight);
        const goalH = Math.floor((xgoal - g.left) / g.cellwidth);
        const goalV = Math.floor((ygoal - g.top) / g.cellheight);
        if (startH < 0 || startH >= g.hcells || startV < 0 || startV >= g.vcells) return false;
        if (goalH < 0 || goalH >= g.hcells || goalV < 0 || goalV >= g.vcells) return false;

        // BFS pathfinding on grid
        const q = [{ h: startH, v: startV }];
        const visited = new Uint8Array(g.hcells * g.vcells);
        const parent = new Int32Array(g.hcells * g.vcells).fill(-1);
        visited[startV * g.hcells + startH] = 1;

        const dirs = allowdiag
            ? [[1,0], [-1,0], [0,1], [0,-1], [1,1], [-1,-1], [1,-1], [-1,1]]
            : [[1,0], [-1,0], [0,1], [0,-1]];

        let found = false;
        while (q.length > 0) {
            const curr = q.shift();
            if (curr.h === goalH && curr.v === goalV) {
                found = true;
                break;
            }
            for (let d = 0; d < dirs.length; d++) {
                const nh = curr.h + dirs[d][0];
                const nv = curr.v + dirs[d][1];
                if (nh >= 0 && nh < g.hcells && nv >= 0 && nv < g.vcells) {
                    const idx = nv * g.hcells + nh;
                    if (!visited[idx] && g.cells[idx] === 0) {
                        visited[idx] = 1;
                        parent[idx] = curr.v * g.hcells + curr.h;
                        q.push({ h: nh, v: nv });
                    }
                }
            }
        }
        if (!found) return false;

        // Reconstruct path
        const cellsPath = [];
        let currIdx = goalV * g.hcells + goalH;
        while (currIdx !== -1) {
            const h = currIdx % g.hcells;
            const v = Math.floor(currIdx / g.hcells);
            cellsPath.push({ x: g.left + (h + 0.5) * g.cellwidth, y: g.top + (v + 0.5) * g.cellheight });
            if (currIdx === startV * g.hcells + startH) break;
            currIdx = parent[currIdx];
        }
        cellsPath.reverse();
        if (cellsPath.length > 0) {
            cellsPath[0] = { x: xstart, y: ystart };
            cellsPath[cellsPath.length - 1] = { x: xgoal, y: ygoal };
        }

        // Write points into path resource
        const pDef = (typeof _get_path_def === "function") ? _get_path_def(pathId) : null;
        if (pDef) {
            pDef.points = cellsPath.map(pt => ({ x: pt.x, y: pt.y, speed: 100 }));
        }
        return true;
    };
    window.mp_linear_path = (pathId, xgoal, ygoal, stepsize, checkall) => {
        const me = window._currentInstance;
        if (!me) return false;
        const pDef = (typeof _get_path_def === "function") ? _get_path_def(pathId) : null;
        if (pDef) {
            pDef.points = [{ x: me.x, y: me.y, speed: 100 }, { x: xgoal, y: ygoal, speed: 100 }];
        }
        return true;
    };
    window.mp_potential_path = (pathId, xgoal, ygoal, stepsize, factor, checkall) => {
        return window.mp_linear_path(pathId, xgoal, ygoal, stepsize, checkall);
    };`;

html = html.substring(0, pMpEnd) + mpGridCode + html.substring(pMpEnd);

// 2. Built-in Special Effects (effect_create_above, effect_create_below, effect_clear)
const targetPartSection = "window.part_system_create = () => {";
const effectCode = `// --- GM8.2 / GML Built-in Effects (effect_create_*) ---
    window.ef_explosion = 0; window.ef_ring = 1; window.ef_ellipse = 2; window.ef_firework = 3;
    window.ef_smoke = 4; window.ef_smokeup = 5; window.ef_star = 6; window.ef_spark = 7;
    window.ef_flare = 8; window.ef_cloud = 9; window.ef_rain = 10; window.ef_snow = 11;
    let _effectSys = null;
    function _ensure_effect_system() {
        if (!_effectSys && typeof window.part_system_create === "function") {
            _effectSys = window.part_system_create();
        }
    }
    window.effect_create_above = (kind, x, y, size, color) => {
        _ensure_effect_system();
        if (!_effectSys) return;
        const colHex = typeof color === "number" ? ("#" + (color & 0xFFFFFF).toString(16).padStart(6, "0")) : (color || "#ffffff");
        const rad = size === 0 ? 8 : (size === 2 ? 24 : 16);
        const count = size === 0 ? 6 : (size === 2 ? 18 : 12);
        for (let i = 0; i < count; i++) {
            const angle = Math.random() * Math.PI * 2;
            const dist = Math.random() * rad;
            if (!window.particles) window.particles = [];
            window.particles.push({
                system: _effectSys,
                x: x + Math.cos(angle) * dist,
                y: y + Math.sin(angle) * dist,
                dx: Math.cos(angle) * (1 + Math.random() * 2),
                dy: Math.sin(angle) * (1 + Math.random() * 2),
                life: 15 + Math.floor(Math.random() * 15),
                col: colHex,
                size: 2 + Math.random() * 2,
                alpha: 1
            });
        }
    };
    window.effect_create_below = window.effect_create_above;
    window.effect_clear = () => {
        if (_effectSys && typeof window.part_system_clear === "function") {
            window.part_system_clear(_effectSys);
        }
    };
    `;

if (!html.includes(targetPartSection)) {
    console.error("targetPartSection not found!");
    process.exit(1);
}
html = html.replace(targetPartSection, effectCode + targetPartSection);

// 3. User Dialogs & Fonts & Env
const targetShowMessage = "window.show_debug_message = (str) => console.log('GML Debug:', str);";
const dialogAndFontCode = `window.show_debug_message = (str) => console.log('GML Debug:', str);
    window.get_string = (promptText, defaultVal = "") => {
        const res = window.prompt(promptText, defaultVal);
        return res !== null ? res : defaultVal;
    };
    window.get_integer = (promptText, defaultVal = 0) => {
        const res = window.prompt(promptText, String(defaultVal));
        const num = parseInt(res, 10);
        return isNaN(num) ? defaultVal : num;
    };
    // --- GML Fonts Management (font_*) ---
    const _gm_fonts = {};
    let _gm_font_counter = 1;
    window.font_add = (name, size, bold, italic, first, last) => {
        const id = _gm_font_counter++;
        const fObj = {
            id,
            name: name || "monospace",
            size: Number(size) || 12,
            bold: !!bold,
            italic: !!italic,
            fontString: (italic ? "italic " : "") + (bold ? "bold " : "") + (Number(size) || 12) + "px " + (name || "monospace")
        };
        _gm_fonts[id] = fObj;
        return id;
    };
    window.font_delete = (id) => { delete _gm_fonts[id]; };
    window.font_exists = (id) => !!_gm_fonts[id] || (window.GAME_DATA?.fonts || []).some(f => f.id === id || f.name === id);
    window.font_get_name = (id) => {
        if (_gm_fonts[id]) return _gm_fonts[id].name;
        const f = (window.GAME_DATA?.fonts || []).find(f => f.id === id || f.name === id);
        return f ? f.name : "";
    };
    // --- GML Environment & Execution Parameters ---
    window.environment_get_variable = (name) => "";
    window.parameter_count = () => 0;
    window.parameter_string = (n) => "";
    window.external_define = () => 0;
    window.external_call = () => 0;
    window.external_free = () => 0;`;

if (!html.includes(targetShowMessage)) {
    console.error("targetShowMessage not found!");
    process.exit(1);
}
html = html.replace(targetShowMessage, dialogAndFontCode);

// 4. Update GMObject prototype prefixes
const targetProtoPrefixes = `const _gmlPrefixes = [
        "draw_", "sound_", "audio_", "instance_", "collision_", "place_", "position_",
        "move_", "motion_", "string_", "ds_", "buffer_", "ini_", "file_", "surface_",
        "gpu_", "part_", "point_", "lengthdir_", "angle_", "dot_product", "darcsin",
        "darccos", "darctan", "dsin", "dcos", "dtan", "clamp", "lerp", "approach",
        "wave", "choose", "random", "irandom", "alarm_"
    ];`;

const newProtoPrefixes = `const _gmlPrefixes = [
        "draw_", "sound_", "audio_", "instance_", "collision_", "place_", "position_",
        "move_", "motion_", "string_", "ds_", "buffer_", "ini_", "file_", "surface_",
        "gpu_", "part_", "point_", "lengthdir_", "angle_", "dot_product", "darcsin",
        "darccos", "darctan", "dsin", "dcos", "dtan", "clamp", "lerp", "approach",
        "wave", "choose", "random", "irandom", "alarm_", "mp_", "effect_", "font_",
        "show_", "get_", "clipboard_", "matrix_", "tile_"
    ];`;

if (!html.includes(targetProtoPrefixes)) {
    console.error("targetProtoPrefixes not found!");
    process.exit(1);
}
html = html.replace(targetProtoPrefixes, newProtoPrefixes);

fs.writeFileSync("app/src/main/assets/www/index.html", html, "utf8");
console.log("All additional GM8.2 core systems successfully integrated via script!");
