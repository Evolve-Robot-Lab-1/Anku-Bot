/**
 * Naval Inspection Robot - Control Panel JavaScript
 *
 * ROS2 communication via rosbridge + dual camera feeds + keyboard controls.
 * Camera feeds use JS fetch + requestAnimationFrame (proven pattern for OBSBOT).
 */

// --- Configuration ---
const CONFIG = {
    rosbridgePort: window.ROSBRIDGE_PORT || 9090,
    camServerPort: window.CAM_SERVER_PORT || 8090,
    cmdVelTopic: '/cmd_vel',
    speedTopic: '/naval/speed',
    estopTopic: '/naval/estop',
    statusTopic: '/naval/status'
};

// --- State ---
let ros = null;
let cmdVelPub = null;
let speedPub = null;
let estopPub = null;
let currentSpeed = 1;
let estopActive = false;
let activeKeys = new Set();
let camBaseUrl = '';

// --- DOM cache ---
const el = {};

// --- Init ---
document.addEventListener('DOMContentLoaded', init);

function init() {
    cacheElements();

    // Build camera base URL
    const host = window.location.hostname;
    camBaseUrl = `http://${host}:${CONFIG.camServerPort}`;

    connectToROS();
    setupMovementButtons();
    setupSpeedButtons();
    setupEstop();
    setupZoomSliders();
    setupKeyboard();
    startCameraFeeds();
}

function cacheElements() {
    el.rosStatus = document.getElementById('ros-status');
    el.camStatus = document.getElementById('cam-status');
    el.camLeft = document.getElementById('cam-left');
    el.camRight = document.getElementById('cam-right');
    el.fpsLeft = document.getElementById('fps-left');
    el.fpsRight = document.getElementById('fps-right');
    el.statusSpeed = document.getElementById('status-speed');
    el.statusDirection = document.getElementById('status-direction');
    el.statusEstop = document.getElementById('status-estop');
    el.statusArduino = document.getElementById('status-arduino');
    el.btnEstop = document.getElementById('btn-estop');
}


// ============================================================
// ROS Connection
// ============================================================

function connectToROS() {
    const host = window.location.hostname;
    const url = `ws://${host}:${CONFIG.rosbridgePort}`;

    ros = new ROSLIB.Ros({ url: url });

    ros.on('connection', () => {
        console.log('Connected to ROS');
        updateRosStatus(true);
        setupPublishers();
        setupSubscribers();
    });

    ros.on('error', (error) => {
        console.error('ROS error:', error);
        updateRosStatus(false);
    });

    ros.on('close', () => {
        console.log('ROS connection closed');
        updateRosStatus(false);
        setTimeout(connectToROS, 3000);
    });
}

function updateRosStatus(connected) {
    el.rosStatus.textContent = connected ? 'ROS: Connected' : 'ROS: Disconnected';
    el.rosStatus.className = 'status-indicator ' + (connected ? 'connected' : 'disconnected');
}

function setupPublishers() {
    cmdVelPub = new ROSLIB.Topic({
        ros: ros,
        name: CONFIG.cmdVelTopic,
        messageType: 'geometry_msgs/Twist'
    });
    cmdVelPub.advertise();

    speedPub = new ROSLIB.Topic({
        ros: ros,
        name: CONFIG.speedTopic,
        messageType: 'std_msgs/Int32'
    });
    speedPub.advertise();

    estopPub = new ROSLIB.Topic({
        ros: ros,
        name: CONFIG.estopTopic,
        messageType: 'std_msgs/Bool'
    });
    estopPub.advertise();
}

function setupSubscribers() {
    const statusSub = new ROSLIB.Topic({
        ros: ros,
        name: CONFIG.statusTopic,
        messageType: 'std_msgs/String'
    });
    statusSub.subscribe((msg) => {
        el.statusArduino.textContent = msg.data;
    });
}


// ============================================================
// Movement Controls
// ============================================================

function sendVelocity(lx, ly, az) {
    if (estopActive || !cmdVelPub) return;

    const twist = new ROSLIB.Message({
        linear:  { x: lx, y: ly, z: 0 },
        angular: { x: 0,  y: 0,  z: az }
    });
    cmdVelPub.publish(twist);

    // Update direction display
    updateDirectionDisplay(lx, ly, az);
}

function updateDirectionDisplay(lx, ly, az) {
    let dir = 'Stopped';
    if (Math.abs(az) > 0.01) {
        dir = az > 0 ? 'Rotate Left' : 'Rotate Right';
    } else if (Math.abs(lx) > 0.01 || Math.abs(ly) > 0.01) {
        if (Math.abs(lx) >= Math.abs(ly)) {
            dir = lx > 0 ? 'Forward' : 'Backward';
        } else {
            dir = ly > 0 ? 'Strafe Left' : 'Strafe Right';
        }
    }
    el.statusDirection.textContent = dir;
}

function stopMoving() {
    sendVelocity(0, 0, 0);
}

function setupMovementButtons() {
    // D-pad: 3x3 grid
    const moves = {
        'btn-forward':    [1, 0, 0],
        'btn-backward':   [-1, 0, 0],
        'btn-strafe-left':  [0, 1, 0],
        'btn-strafe-right': [0, -1, 0],
        'btn-rot-left':   [0, 0, 1],
        'btn-rot-right':  [0, 0, -1],
        'btn-fwd-left':   [1, 1, 0],
        'btn-fwd-right':  [1, -1, 0],
        'btn-bwd-left':   [-1, 1, 0],
        'btn-bwd-right':  [-1, -1, 0]
    };

    Object.keys(moves).forEach(id => {
        const btn = document.getElementById(id);
        if (!btn) return;
        const [lx, ly, az] = moves[id];

        // Mouse
        btn.addEventListener('mousedown', () => sendVelocity(lx, ly, az));
        btn.addEventListener('mouseup', stopMoving);
        btn.addEventListener('mouseleave', stopMoving);

        // Touch
        btn.addEventListener('touchstart', (e) => {
            e.preventDefault();
            sendVelocity(lx, ly, az);
        });
        btn.addEventListener('touchend', stopMoving);
        btn.addEventListener('touchcancel', stopMoving);
    });

    // Stop button
    document.getElementById('btn-stop').addEventListener('click', stopMoving);
}


// ============================================================
// Speed Control
// ============================================================

function setSpeed(level) {
    currentSpeed = level;

    if (speedPub) {
        speedPub.publish(new ROSLIB.Message({ data: level }));
    }

    // Update UI
    document.querySelectorAll('.btn-speed').forEach(btn => btn.classList.remove('active'));
    const activeBtn = document.getElementById('btn-speed-' + level);
    if (activeBtn) activeBtn.classList.add('active');
    el.statusSpeed.textContent = level;
}

function setupSpeedButtons() {
    [1, 2, 3].forEach(level => {
        const btn = document.getElementById('btn-speed-' + level);
        if (btn) btn.addEventListener('click', () => setSpeed(level));
    });
}


// ============================================================
// E-STOP
// ============================================================

function toggleEstop() {
    estopActive = !estopActive;

    if (estopPub) {
        estopPub.publish(new ROSLIB.Message({ data: estopActive }));
    }

    if (estopActive) {
        stopMoving();
        el.btnEstop.textContent = 'RELEASE E-STOP';
        el.btnEstop.classList.add('btn-estop-active');
        el.statusEstop.textContent = 'ACTIVE';
        el.statusEstop.style.color = '#f44336';
    } else {
        el.btnEstop.textContent = 'E-STOP';
        el.btnEstop.classList.remove('btn-estop-active');
        el.statusEstop.textContent = 'OFF';
        el.statusEstop.style.color = '';
    }
}

function setupEstop() {
    el.btnEstop.addEventListener('click', toggleEstop);
}


// ============================================================
// Camera Feeds (JS fetch + requestAnimationFrame)
// ============================================================

function startCameraFeeds() {
    startFeed('cam-left', `${camBaseUrl}/frame/left`, 'fps-left');
    startFeed('cam-right', `${camBaseUrl}/frame/right`, 'fps-right');

    // Check camera server health
    checkCameraHealth();
}

function startFeed(imgId, url, fpsId) {
    const img = document.getElementById(imgId);
    const fpsEl = document.getElementById(fpsId);
    let frames = 0;
    let lastTime = Date.now();

    function update() {
        const newImg = new Image();
        newImg.onload = function() {
            img.src = newImg.src;
            frames++;
            const now = Date.now();
            if (now - lastTime >= 1000) {
                fpsEl.textContent = frames + ' FPS';
                frames = 0;
                lastTime = now;
            }
            requestAnimationFrame(update);
        };
        newImg.onerror = function() {
            setTimeout(update, 100);
        };
        newImg.src = url + '?t=' + Date.now();
    }
    update();
}

function checkCameraHealth() {
    fetch(`${camBaseUrl}/health`)
        .then(r => r.json())
        .then(data => {
            el.camStatus.textContent = 'CAM: Connected';
            el.camStatus.className = 'status-indicator connected';
        })
        .catch(() => {
            el.camStatus.textContent = 'CAM: Disconnected';
            el.camStatus.className = 'status-indicator disconnected';
        });

    // Re-check every 5 seconds
    setTimeout(checkCameraHealth, 5000);
}


// ============================================================
// Zoom Control
// ============================================================

function setupZoomSliders() {
    const zoomLeft = document.getElementById('zoom-left');
    const zoomRight = document.getElementById('zoom-right');
    const zoomLeftVal = document.getElementById('zoom-left-val');
    const zoomRightVal = document.getElementById('zoom-right-val');

    if (zoomLeft) {
        zoomLeft.addEventListener('input', (e) => {
            zoomLeftVal.textContent = e.target.value;
        });
        zoomLeft.addEventListener('change', (e) => {
            sendZoom('left', parseInt(e.target.value));
        });
    }

    if (zoomRight) {
        zoomRight.addEventListener('input', (e) => {
            zoomRightVal.textContent = e.target.value;
        });
        zoomRight.addEventListener('change', (e) => {
            sendZoom('right', parseInt(e.target.value));
        });
    }
}

function sendZoom(side, level) {
    fetch(`${camBaseUrl}/zoom/${side}`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ level: level })
    }).catch(err => console.error('Zoom error:', err));
}


// ============================================================
// Keyboard Controls
// ============================================================

function setupKeyboard() {
    document.addEventListener('keydown', handleKeyDown);
    document.addEventListener('keyup', handleKeyUp);
}

function handleKeyDown(event) {
    // Ignore if typing in an input
    if (event.target.tagName === 'INPUT' || event.target.tagName === 'TEXTAREA') return;
    if (event.repeat) return;

    const key = event.key.toLowerCase();
    activeKeys.add(key);

    switch (key) {
        case 'w': sendVelocity(1, 0, 0); break;
        case 's': sendVelocity(-1, 0, 0); break;
        case 'a': sendVelocity(0, 1, 0); break;
        case 'd': sendVelocity(0, -1, 0); break;
        case 'q': sendVelocity(0, 0, 1); break;
        case 'e': sendVelocity(0, 0, -1); break;
        case '1': setSpeed(1); break;
        case '2': setSpeed(2); break;
        case '3': setSpeed(3); break;
        case ' ':
            event.preventDefault();
            stopMoving();
            break;
        case 'escape':
            toggleEstop();
            break;
    }
}

function handleKeyUp(event) {
    const key = event.key.toLowerCase();
    activeKeys.delete(key);

    if (['w', 's', 'a', 'd', 'q', 'e'].includes(key)) {
        // If no movement keys held, stop
        const moveKeys = ['w', 's', 'a', 'd', 'q', 'e'];
        const anyHeld = moveKeys.some(k => activeKeys.has(k));
        if (!anyHeld) {
            stopMoving();
        }
    }
}
