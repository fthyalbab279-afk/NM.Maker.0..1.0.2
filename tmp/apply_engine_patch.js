const fs = require('fs');

let html = fs.readFileSync('app/src/main/assets/www/index.html', 'utf8');

// 1. Upgrade syncKeyboardToPlayers
const syncStart = html.indexOf('const syncKeyboardToPlayers = (keyCodeOrName, down) => {');
const syncEnd = html.indexOf('window.syncKeyboardToPlayers = syncKeyboardToPlayers;', syncStart);

if (syncStart !== -1 && syncEnd !== -1) {
    const syncOld = html.substring(syncStart, syncEnd);
    const syncNew = `const syncKeyboardToPlayers = (keyCodeOrName, down) => {
        let name = keyCodeOrName;
        let code = typeof keyCodeOrName === "number" ? keyCodeOrName : null;
        if (typeof keyCodeOrName === "number") {
            name = mapGMKey(keyCodeOrName) || "";
        } else if (typeof keyCodeOrName === "string") {
            const consts = window.vk_constants || {};
            for (let k in consts) {
                if (consts[k] === keyCodeOrName || String(consts[k]).toLowerCase() === keyCodeOrName.toLowerCase()) {
                    code = Number(k);
                    break;
                }
            }
        }

        // Direct sync
        if (keyCodeOrName !== undefined && keyCodeOrName !== null) {
            P1_Input.syncKey(keyCodeOrName, down);
        }
        if (name && name !== keyCodeOrName) {
            P1_Input.syncKey(name, down);
        }
        if (code !== null && code !== keyCodeOrName) {
            P1_Input.syncKey(code, down);
        }

        // Player 1 mapping: Directional & Combos
        if (name === "ArrowLeft" || code === 37) {
            P1_Input.syncKey("ArrowLeft", down);
            P1_Input.syncKey(37, down);
            P1_Input.syncKey("left", down);
        } else if (name === "ArrowRight" || code === 39) {
            P1_Input.syncKey("ArrowRight", down);
            P1_Input.syncKey(39, down);
            P1_Input.syncKey("right", down);
        } else if (name === "ArrowUp" || code === 38) {
            P1_Input.syncKey("ArrowUp", down);
            P1_Input.syncKey(38, down);
            P1_Input.syncKey("up", down);
        } else if (name === "ArrowDown" || code === 40) {
            P1_Input.syncKey("ArrowDown", down);
            P1_Input.syncKey(40, down);
            P1_Input.syncKey("down", down);
        } else if (name === "Space" || name === " " || code === 32 || name === "KeyZ" || name === "z" || name === "Z" || code === 90) {
            // Jump / Primary action (triggers Space, Z, and ArrowUp for platformers)
            P1_Input.syncKey("Space", down);
            P1_Input.syncKey(32, down);
            P1_Input.syncKey("space", down);
            P1_Input.syncKey("KeyZ", down);
            P1_Input.syncKey(90, down);
            P1_Input.syncKey("z", down);
            P1_Input.syncKey(" ", down);
            P1_Input.syncKey("ArrowUp", down);
            P1_Input.syncKey(38, down);
            P1_Input.syncKey("up", down);
        } else if (name === "KeyX" || name === "x" || name === "X" || code === 88 || name === "Shift" || name === "ShiftLeft" || code === 16 || name === "Control" || name === "ControlLeft" || code === 17) {
            // Action / Run / Shoot
            P1_Input.syncKey("KeyX", down);
            P1_Input.syncKey(88, down);
            P1_Input.syncKey("x", down);
            P1_Input.syncKey("Shift", down);
            P1_Input.syncKey("ShiftLeft", down);
            P1_Input.syncKey(16, down);
            P1_Input.syncKey("Control", down);
            P1_Input.syncKey("ControlLeft", down);
            P1_Input.syncKey(17, down);
            P1_Input.syncKey("KeyC", down);
            P1_Input.syncKey(67, down);
            P1_Input.syncKey("c", down);
        }

        // Player 2 mapping
        if (name === "KeyA" || name === "KeyJ") {
            P2_Input.syncKey("ArrowLeft", down);
            P2_Input.syncKey("KeyA", down);
            P2_Input.syncKey("a", down);
        } else if (name === "KeyD" || name === "KeyL") {
            P2_Input.syncKey("ArrowRight", down);
            P2_Input.syncKey("KeyD", down);
            P2_Input.syncKey("d", down);
        } else if (name === "KeyW" || name === "KeyI") {
            P2_Input.syncKey("ArrowUp", down);
            P2_Input.syncKey("KeyW", down);
            P2_Input.syncKey("w", down);
        } else if (name === "KeyS" || name === "KeyK") {
            P2_Input.syncKey("ArrowDown", down);
            P2_Input.syncKey("KeyS", down);
            P2_Input.syncKey("s", down);
        } else if (name === "KeyF" || name === "ShiftLeft") {
            P2_Input.syncKey("Space", down);
            P2_Input.syncKey("KeyZ", down);
            P2_Input.syncKey("z", down);
            P2_Input.syncKey(" ", down);
            P2_Input.syncKey("ArrowUp", down);
        } else if (name === "KeyG") {
            P2_Input.syncKey("KeyX", down);
            P2_Input.syncKey("x", down);
        } else {
            P2_Input.syncKey(name, down);
        }
    };
    `;
    html = html.replace(syncOld, syncNew);
    console.log("Replaced syncKeyboardToPlayers successfully");
} else {
    console.error("Could not find syncKeyboardToPlayers range");
}

// 2. Upgrade GMObject.triggerEvent keyboard matching
const trigStart = html.indexOf("if (['keyboard', 'keypress', 'keyrelease'].includes(type)) {");
const trigEnd = html.indexOf("if (type === 'mouse') eventKey = type + '_' + data;", trigStart);

if (trigStart !== -1 && trigEnd !== -1) {
    const trigOld = html.substring(trigStart, trigEnd);
    const trigNew = `if (['keyboard', 'keypress', 'keyrelease'].includes(type)) {
                const evMap = OBJECT_EVENTS[this.def.id];
                if (!evMap) return;
                const KEY_ALIASES = {
                    'ArrowLeft': [37, 'left', 'ArrowLeft', 'vk_left', 'Left'],
                    'ArrowRight': [39, 'right', 'ArrowRight', 'vk_right', 'Right'],
                    'ArrowUp': [38, 'up', 'ArrowUp', 'vk_up', 'Up'],
                    'ArrowDown': [40, 'down', 'ArrowDown', 'vk_down', 'Down'],
                    'Space': [32, 'space', 'Space', 'vk_space', ' '],
                    ' ': [32, 'space', 'Space', 'vk_space', ' '],
                    'Enter': [13, 'enter', 'Enter', 'vk_enter'],
                    'Shift': [16, 'shift', 'Shift', 'ShiftLeft', 'ShiftRight', 'vk_shift'],
                    'ShiftLeft': [16, 'shift', 'Shift', 'ShiftLeft', 'ShiftRight', 'vk_shift'],
                    'Control': [17, 'control', 'Control', 'ControlLeft', 'ControlRight', 'vk_control'],
                    'ControlLeft': [17, 'control', 'Control', 'ControlLeft', 'ControlRight', 'vk_control'],
                    'Escape': [27, 'escape', 'Escape', 'vk_escape'],
                    'KeyZ': [90, 'KeyZ', 'z', 'Z'], 'z': [90, 'KeyZ', 'z', 'Z'], 'Z': [90, 'KeyZ', 'z', 'Z'],
                    'KeyX': [88, 'KeyX', 'x', 'X'], 'x': [88, 'KeyX', 'x', 'X'], 'X': [88, 'KeyX', 'x', 'X'],
                    'KeyC': [67, 'KeyC', 'c', 'C'], 'c': [67, 'KeyC', 'c', 'C'], 'C': [67, 'KeyC', 'c', 'C'],
                    'KeyA': [65, 'KeyA', 'a', 'A'], 'a': [65, 'KeyA', 'a', 'A'], 'A': [65, 'KeyA', 'a', 'A'],
                    'KeyD': [68, 'KeyD', 'd', 'D'], 'd': [68, 'KeyD', 'd', 'D'], 'D': [68, 'KeyD', 'd', 'D'],
                    'KeyW': [87, 'KeyW', 'w', 'W'], 'w': [87, 'KeyW', 'w', 'W'], 'W': [87, 'KeyW', 'w', 'W'],
                    'KeyS': [83, 'KeyS', 's', 'S'], 's': [83, 'KeyS', 's', 'S'], 'S': [83, 'KeyS', 's', 'S'],
                };
                let candidates = [];
                if (data !== null && data !== undefined) candidates.push(data);
                const name = typeof data === 'number' ? mapGMKey(data) : data;
                if (name && !candidates.includes(name)) candidates.push(name);
                if (KEY_ALIASES[data]) candidates.push(...KEY_ALIASES[data]);
                if (name && KEY_ALIASES[name]) candidates.push(...KEY_ALIASES[name]);
                if (typeof data === 'number') {
                    const consts = window.vk_constants || {};
                    if (consts[data]) candidates.push(consts[data]);
                } else if (typeof data === 'string') {
                    if (data.startsWith('Key') && data.length === 4) {
                        const ch = data.charAt(3);
                        candidates.push(ch, ch.toLowerCase(), ch.toUpperCase(), ch.charCodeAt(0));
                    } else if (data.length === 1) {
                        candidates.push(data.charCodeAt(0), 'Key' + data.toUpperCase());
                    }
                    const consts = window.vk_constants || {};
                    for (let code in consts) {
                        if (consts[code] === data || String(consts[code]).toLowerCase() === data.toLowerCase()) {
                            candidates.push(Number(code));
                        }
                    }
                }
                let resolved = null;
                for (let c of candidates) {
                    if (evMap[type + '_' + c]) { resolved = type + '_' + c; break; }
                    if (evMap[type + '_' + String(c).toLowerCase()]) { resolved = type + '_' + String(c).toLowerCase(); break; }
                }
                if (resolved) {
                    eventKey = resolved;
                } else {
                    return;
                }
            }
            `;
    html = html.replace(trigOld, trigNew);
    console.log("Replaced triggerEvent keyboard handling successfully");
} else {
    console.error("Could not find triggerEvent range");
}

// 3. Upgrade GMObject.update physics sync and key triggers
const gmxKbStart = html.indexOf("// --- GMX KEYBOARD EVENTS ---");
const gmxKbEnd = html.indexOf("// Apply gravity along gravity_direction (GML: 270=down)", gmxKbStart);

if (gmxKbStart !== -1 && gmxKbEnd !== -1) {
    const gmxOld = html.substring(gmxKbStart, gmxKbEnd);
    const gmxNew = `// --- GMX KEYBOARD EVENTS ---
            // Trigger events for all active keys (both numeric and named)
            const currentInput = this.playerIndex === 1 ? window.P2_Input : window.P1_Input;
            const activeKeys = new Set();
            for (let k in currentInput.keys) {
                if (currentInput.keys[k]) {
                    const name = typeof k === 'number' || !isNaN(Number(k)) ? mapGMKey(Number(k)) : k;
                    if (name) activeKeys.add(name);
                    activeKeys.add(k);
                }
            }
            activeKeys.forEach(name => this.triggerEvent('keyboard', name));
            const pressedKeys = new Set();
            for (let k in currentInput.keysPressed) {
                if (currentInput.keysPressed[k]) {
                    const name = typeof k === 'number' || !isNaN(Number(k)) ? mapGMKey(Number(k)) : k;
                    if (name) pressedKeys.add(name);
                    pressedKeys.add(k);
                }
            }
            pressedKeys.forEach(name => this.triggerEvent('keypress', name));
            const releasedKeys = new Set();
            for (let k in currentInput.keysReleased) {
                if (currentInput.keysReleased[k]) {
                    const name = typeof k === 'number' || !isNaN(Number(k)) ? mapGMKey(Number(k)) : k;
                    if (name) releasedKeys.add(name);
                    releasedKeys.add(k);
                }
            }
            releasedKeys.forEach(name => this.triggerEvent('keyrelease', name));

            // Sync any changes to hspeed/vspeed/speed made by Step or Keyboard events into dx/dy
            if (this.hspeed !== this.dx) this.dx = this.hspeed;
            if (this.vspeed !== this.dy) this.dy = this.vspeed;
            if (this.speed !== 0 && this.speed !== Math.hypot(this.dx, this.dy)) {
                this.dx = this.speed * Math.cos(this.direction * Math.PI / 180);
                this.dy = -this.speed * Math.sin(this.direction * Math.PI / 180);
            }
            `;
    html = html.replace(gmxOld, gmxNew);
    console.log("Replaced GMObject.update physics sync successfully");
} else {
    console.error("Could not find GMX KEYBOARD EVENTS range");
}

fs.writeFileSync('app/src/main/assets/www/index.html', html, 'utf8');
console.log("Engine patch saved.");
