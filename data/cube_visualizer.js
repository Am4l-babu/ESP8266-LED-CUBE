// ==========================================
// cube_visualizer.js - Three.js 3D LED Cube
// Interactive 3D visualization synchronized
// with the real LED cube via WebSocket
// ==========================================

let cubeScene, cubeCamera, cubeRenderer, cubeControls;
let ledSpheres = [];
let cubeInitialized = false;
let animFrameId = null;

// LED colors
const LED_ON_COLOR = new THREE.Color(0x00e5ff);
const LED_OFF_COLOR = new THREE.Color(0x111122);
const LED_ON_EMISSIVE = new THREE.Color(0x00aacc);
const LED_OFF_EMISSIVE = new THREE.Color(0x000000);

function initCubeVisualizer() {
    if (cubeInitialized) {
        handleResize();
        return;
    }

    const container = document.getElementById('cubeCanvas');
    if (!container) return;

    const w = container.clientWidth;
    const h = container.clientHeight;

    // Scene
    cubeScene = new THREE.Scene();
    cubeScene.fog = new THREE.Fog(0x050510, 15, 30);

    // Camera
    cubeCamera = new THREE.PerspectiveCamera(45, w / h, 0.1, 100);
    cubeCamera.position.set(6, 5, 8);
    cubeCamera.lookAt(0, 0, 0);

    // Renderer
    cubeRenderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
    cubeRenderer.setSize(w, h);
    cubeRenderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    cubeRenderer.setClearColor(0x050510);
    container.appendChild(cubeRenderer.domElement);

    // Lights
    const ambient = new THREE.AmbientLight(0x222244, 0.5);
    cubeScene.add(ambient);

    const point1 = new THREE.PointLight(0x00e5ff, 0.5, 20);
    point1.position.set(5, 5, 5);
    cubeScene.add(point1);

    const point2 = new THREE.PointLight(0xff00e5, 0.3, 20);
    point2.position.set(-5, 3, -5);
    cubeScene.add(point2);

    // Grid helper
    const gridSize = 6;
    const grid = new THREE.GridHelper(gridSize, 8, 0x1a1a3a, 0x0d0d2b);
    grid.position.y = -2;
    cubeScene.add(grid);

    // Create LED spheres (4x4x4)
    const spacing = 1.5;
    const offset = (3 * spacing) / 2;

    const sphereGeo = new THREE.SphereGeometry(0.2, 16, 16);

    for (let z = 0; z < 4; z++) {
        for (let y = 0; y < 4; y++) {
            for (let x = 0; x < 4; x++) {
                const mat = new THREE.MeshStandardMaterial({
                    color: LED_OFF_COLOR.clone(),
                    emissive: LED_OFF_EMISSIVE.clone(),
                    emissiveIntensity: 0,
                    metalness: 0.3,
                    roughness: 0.4,
                    transparent: true,
                    opacity: 0.6
                });

                const sphere = new THREE.Mesh(sphereGeo, mat);
                sphere.position.set(
                    x * spacing - offset,
                    z * spacing - offset,
                    y * spacing - offset
                );
                sphere.userData = { x, y, z, on: false };

                cubeScene.add(sphere);
                ledSpheres.push(sphere);
            }
        }
    }

    // Draw connection wires (columns)
    const wireMat = new THREE.LineBasicMaterial({ 
        color: 0x1a1a3a, 
        transparent: true, 
        opacity: 0.3 
    });

    for (let x = 0; x < 4; x++) {
        for (let y = 0; y < 4; y++) {
            const points = [];
            for (let z = 0; z < 4; z++) {
                points.push(new THREE.Vector3(
                    x * spacing - offset,
                    z * spacing - offset,
                    y * spacing - offset
                ));
            }
            const geo = new THREE.BufferGeometry().setFromPoints(points);
            const line = new THREE.Line(geo, wireMat);
            cubeScene.add(line);
        }
    }

    // Draw layer planes (horizontal wires)
    for (let z = 0; z < 4; z++) {
        for (let x = 0; x < 4; x++) {
            const points = [];
            for (let y = 0; y < 4; y++) {
                points.push(new THREE.Vector3(
                    x * spacing - offset,
                    z * spacing - offset,
                    y * spacing - offset
                ));
            }
            const geo = new THREE.BufferGeometry().setFromPoints(points);
            const line = new THREE.Line(geo, wireMat);
            cubeScene.add(line);
        }
        for (let y = 0; y < 4; y++) {
            const points = [];
            for (let x = 0; x < 4; x++) {
                points.push(new THREE.Vector3(
                    x * spacing - offset,
                    z * spacing - offset,
                    y * spacing - offset
                ));
            }
            const geo = new THREE.BufferGeometry().setFromPoints(points);
            const line = new THREE.Line(geo, wireMat);
            cubeScene.add(line);
        }
    }

    // Simple orbit controls (manual implementation)
    setupOrbitControls(container);

    // Handle resize
    window.addEventListener('resize', handleResize);

    cubeInitialized = true;

    // Start render loop
    animate();
}

function handleResize() {
    const container = document.getElementById('cubeCanvas');
    if (!container || !cubeRenderer) return;

    const w = container.clientWidth;
    const h = container.clientHeight;

    cubeCamera.aspect = w / h;
    cubeCamera.updateProjectionMatrix();
    cubeRenderer.setSize(w, h);
}

// Simple orbit controls
function setupOrbitControls(container) {
    let isDragging = false;
    let prevX = 0, prevY = 0;
    let theta = 0.8, phi = 0.6;
    let radius = 10;

    function updateCamera() {
        cubeCamera.position.x = radius * Math.sin(phi) * Math.cos(theta);
        cubeCamera.position.y = radius * Math.cos(phi);
        cubeCamera.position.z = radius * Math.sin(phi) * Math.sin(theta);
        cubeCamera.lookAt(0, 0, 0);
    }

    function onPointerDown(e) {
        isDragging = true;
        prevX = e.clientX || (e.touches && e.touches[0].clientX);
        prevY = e.clientY || (e.touches && e.touches[0].clientY);
    }

    function onPointerMove(e) {
        if (!isDragging) return;
        const x = e.clientX || (e.touches && e.touches[0].clientX);
        const y = e.clientY || (e.touches && e.touches[0].clientY);
        
        theta += (x - prevX) * 0.01;
        phi -= (y - prevY) * 0.01;
        phi = Math.max(0.1, Math.min(Math.PI - 0.1, phi));
        
        prevX = x;
        prevY = y;
        updateCamera();
    }

    function onPointerUp() {
        isDragging = false;
    }

    function onWheel(e) {
        e.preventDefault();
        radius += e.deltaY * 0.01;
        radius = Math.max(4, Math.min(20, radius));
        updateCamera();
    }

    container.addEventListener('mousedown', onPointerDown);
    container.addEventListener('mousemove', onPointerMove);
    container.addEventListener('mouseup', onPointerUp);
    container.addEventListener('mouseleave', onPointerUp);
    container.addEventListener('wheel', onWheel, { passive: false });

    // Touch support
    container.addEventListener('touchstart', onPointerDown, { passive: true });
    container.addEventListener('touchmove', onPointerMove, { passive: true });
    container.addEventListener('touchend', onPointerUp);

    updateCamera();
}

function animate() {
    animFrameId = requestAnimationFrame(animate);

    // Subtle auto-rotation when not interacting
    // (commented out, enable if desired)
    // cubeCamera.position.applyAxisAngle(new THREE.Vector3(0, 1, 0), 0.002);
    // cubeCamera.lookAt(0, 0, 0);

    // Animate LED glow pulsing for ON LEDs
    ledSpheres.forEach(sphere => {
        if (sphere.userData.on) {
            const pulse = 0.8 + 0.2 * Math.sin(Date.now() * 0.003 + sphere.userData.x + sphere.userData.z);
            sphere.material.emissiveIntensity = pulse;
        }
    });

    if (cubeRenderer && cubeScene && cubeCamera) {
        cubeRenderer.render(cubeScene, cubeCamera);
    }
}

// Update LED states from WebSocket data
function updateCubeVisualizer(layers) {
    if (!cubeInitialized || !layers) return;

    for (let z = 0; z < 4; z++) {
        const layerData = layers[z] || 0;
        for (let y = 0; y < 4; y++) {
            for (let x = 0; x < 4; x++) {
                const bitPos = y * 4 + x;
                const isOn = (layerData >> bitPos) & 1;
                const idx = z * 16 + y * 4 + x;
                const sphere = ledSpheres[idx];

                if (sphere && sphere.userData.on !== !!isOn) {
                    sphere.userData.on = !!isOn;
                    if (isOn) {
                        sphere.material.color.copy(LED_ON_COLOR);
                        sphere.material.emissive.copy(LED_ON_EMISSIVE);
                        sphere.material.emissiveIntensity = 1;
                        sphere.material.opacity = 1;
                    } else {
                        sphere.material.color.copy(LED_OFF_COLOR);
                        sphere.material.emissive.copy(LED_OFF_EMISSIVE);
                        sphere.material.emissiveIntensity = 0;
                        sphere.material.opacity = 0.6;
                    }
                }
            }
        }
    }
}
