// ==========================================
// voxel_editor.js - Voxel Pattern Editor
// Layer-by-layer 4x4 grid for toggling LEDs
// Save/Load patterns to ESP via WebSocket
// ==========================================

let editorBuffer = [0, 0, 0, 0];  // 4 layers, 16 bits each
let currentEditorLayer = 3;        // Start at top layer
let selectedSlot = 0;

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
                toggleEditorVoxel(x, y);
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

function updateEditorGrid() {
    const cells = document.querySelectorAll('.voxel-cell');
    const layerData = editorBuffer[currentEditorLayer] || 0;

    cells.forEach(cell => {
        const x = parseInt(cell.dataset.x);
        const y = parseInt(cell.dataset.y);
        const bitPos = y * 4 + x;
        const isOn = (layerData >> bitPos) & 1;
        cell.classList.toggle('on', !!isOn);
    });
}

function updateEditorFromState(layers) {
    if (!layers || layers.length !== 4) return;
    editorBuffer = [...layers];
    updateEditorGrid();
}

function setupEditorControls() {
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
