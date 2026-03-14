// ==========================================
// voxel_editor.js - Voxel Pattern Editor
// Layer-by-layer 4x4 grid for toggling LEDs
// Save/Load patterns to ESP via WebSocket
// ==========================================

let editorBuffer = [0, 0, 0, 0];  // 4 layers, 16 bits each
let currentEditorLayer = 3;        // Start at top layer
let selectedSlot = 0;

// Hardware color painting state
let editorMode = 'pattern'; // 'pattern' or 'paint'
let hardwareColorMap = new Array(64).fill('#00e5ff');
let currentPaintColor = '#00e5ff';

// Initialize the voxel editor grid
function initVoxelEditor() {
    const grid = document.getElementById('voxelGrid');
    if (!grid) return;

    grid.innerHTML = '';

    for (let y = 0; y < 4; y++) {
        for (let x = 0; x < 4; x++) {
            const cell = document.createElement('div');
            cell.className = 'voxel-cell';
            cell.dataset.x = x;
            cell.dataset.y = y;

            const coord = document.createElement('span');
            coord.className = 'coord';
            coord.textContent = `${x},${y}`;
            cell.appendChild(coord);

            cell.addEventListener('click', () => {
                if (editorMode === 'pattern') {
                    toggleEditorVoxel(x, y);
                } else if (editorMode === 'paint') {
                    paintEditorVoxel(x, y);
                }
            });

            grid.appendChild(cell);
        }
    }

    updateEditorGrid();
    setupEditorControls();
}

function toggleEditorVoxel(x, y) {
    const bitPos = y * 4 + x;
    editorBuffer[currentEditorLayer] ^= (1 << bitPos);
    updateEditorGrid();
}

function paintEditorVoxel(x, y) {
    const idx = currentEditorLayer * 16 + y * 4 + x;
    hardwareColorMap[idx] = currentPaintColor;
    updateEditorGrid();
}

function updateEditorGrid() {
    const cells = document.querySelectorAll('.voxel-cell');
    const layerData = editorBuffer[currentEditorLayer] || 0;

    cells.forEach(cell => {
        const x = parseInt(cell.dataset.x);
        const y = parseInt(cell.dataset.y);
        const bitPos = y * 4 + x;
        const idx = currentEditorLayer * 16 + y * 4 + x;
        
        const isOn = (layerData >> bitPos) & 1;
        cell.classList.toggle('on', !!isOn);
        
        // Update CSS variable for the cell's color
        cell.style.setProperty('--led-color-cell', hardwareColorMap[idx]);
    });
}

function updateEditorFromState(layers) {
    if (!layers || layers.length !== 4) return;
    editorBuffer = [...layers];
    updateEditorGrid();
}

function setupEditorControls() {
    // --- Mode Toggle ---
    const modePattern = document.getElementById('modePattern');
    const modePaint = document.getElementById('modePaint');
    const patternControls = document.getElementById('patternControls');
    const paintControls = document.getElementById('paintControls');

    if (modePattern && modePaint) {
        modePattern.addEventListener('click', () => {
            editorMode = 'pattern';
            modePattern.classList.add('active');
            modePaint.classList.remove('active');
            patternControls.classList.remove('hidden');
            paintControls.classList.add('hidden');
        });
        modePaint.addEventListener('click', () => {
            editorMode = 'paint';
            modePaint.classList.add('active');
            modePattern.classList.remove('active');
            paintControls.classList.remove('hidden');
            patternControls.classList.add('hidden');
        });
    }

    // --- Paint Controls ---
    const paintColorPicker = document.getElementById('paintColorPicker');
    if (paintColorPicker) {
        paintColorPicker.addEventListener('input', (e) => {
            currentPaintColor = e.target.value;
            document.getElementById('paintHexValue').textContent = currentPaintColor.toUpperCase();
        });
    }

    document.getElementById('paintFillLayer')?.addEventListener('click', () => {
        const startIdx = currentEditorLayer * 16;
        for (let i = 0; i < 16; i++) hardwareColorMap[startIdx + i] = currentPaintColor;
        updateEditorGrid();
    });

    document.getElementById('paintFillAll')?.addEventListener('click', () => {
        hardwareColorMap.fill(currentPaintColor);
        updateEditorGrid();
    });

    document.getElementById('paintSave')?.addEventListener('click', () => {
        fetch('/api/colors', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ colors: hardwareColorMap })
        })
        .then(response => response.json())
        .then(data => {
            console.log('Colors saved:', data);
            // Alert user visually?
            const btn = document.getElementById('paintSave');
            const originalText = btn.textContent;
            btn.textContent = 'Saved!';
            setTimeout(() => btn.textContent = originalText, 2000);
        })
        .catch(err => console.error('Failed to save colors:', err));
    });

    // Fetch initial colors
    fetch('/api/colors')
        .then(res => res.json())
        .then(data => {
            if (data && data.colors && data.colors.length === 64) {
                hardwareColorMap = data.colors;
                if (typeof updateHardwareColors === 'function') {
                    updateHardwareColors(hardwareColorMap);
                }
                updateEditorGrid();
            }
        })
        .catch(err => console.error('Failed to load colors:', err));

    // --- Original Pattern Controls (Clear/Fill/Send) ---
    // Layer tabs
    document.querySelectorAll('.layer-tab').forEach(tab => {
        tab.addEventListener('click', () => {
            currentEditorLayer = parseInt(tab.dataset.layer);
            document.querySelectorAll('.layer-tab').forEach(t => t.classList.remove('active'));
            tab.classList.add('active');
            updateEditorGrid();
        });
    });

    // Clear all
    const clearBtn = document.getElementById('editorClear');
    if (clearBtn) {
        clearBtn.addEventListener('click', () => {
            editorBuffer = [0, 0, 0, 0];
            updateEditorGrid();
        });
    }

    // Fill all
    const fillBtn = document.getElementById('editorFill');
    if (fillBtn) {
        fillBtn.addEventListener('click', () => {
            editorBuffer = [0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF];
            updateEditorGrid();
        });
    }

    // Send to cube
    const sendBtn = document.getElementById('editorSend');
    if (sendBtn) {
        sendBtn.addEventListener('click', () => {
            // Stop any running animation first
            sendCmd('stop');
            // Send buffer
            sendCmd('setBuffer', { layers: editorBuffer });
        });
    }

    // Pattern slot selection
    document.querySelectorAll('.slot-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            selectedSlot = parseInt(btn.dataset.slot);
            document.querySelectorAll('.slot-btn').forEach(b => b.classList.remove('selected'));
            btn.classList.add('selected');
        });
    });

    // Select first slot by default
    const firstSlot = document.querySelector('.slot-btn[data-slot="0"]');
    if (firstSlot) firstSlot.classList.add('selected');

    // Save pattern
    const saveBtn = document.getElementById('savePatternBtn');
    if (saveBtn) {
        saveBtn.addEventListener('click', () => {
            // First send the buffer to cube, then save
            sendCmd('setBuffer', { layers: editorBuffer });
            setTimeout(() => {
                sendCmd('savePattern', { slot: selectedSlot });
            }, 100);
        });
    }

    // Load pattern
    const loadBtn = document.getElementById('loadPatternBtn');
    if (loadBtn) {
        loadBtn.addEventListener('click', () => {
            sendCmd('loadPattern', { slot: selectedSlot });
        });
    }

    // Export pattern as JSON
    const exportBtn = document.getElementById('exportPatternBtn');
    if (exportBtn) {
        exportBtn.addEventListener('click', () => {
            const data = {
                name: `Pattern ${selectedSlot + 1}`,
                layers: editorBuffer,
                voxels: []
            };

            // Generate human-readable voxel list
            for (let z = 0; z < 4; z++) {
                for (let y = 0; y < 4; y++) {
                    for (let x = 0; x < 4; x++) {
                        const bitPos = y * 4 + x;
                        if ((editorBuffer[z] >> bitPos) & 1) {
                            data.voxels.push({ x, y, z });
                        }
                    }
                }
            }

            const json = JSON.stringify(data, null, 2);
            
            // Copy to clipboard
            navigator.clipboard.writeText(json).then(() => {
                alert('Pattern JSON copied to clipboard!');
            }).catch(() => {
                // Fallback: show in prompt
                prompt('Copy this pattern JSON:', json);
            });
        });
    }
}

// Auto-initialize when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    initVoxelEditor();
});
