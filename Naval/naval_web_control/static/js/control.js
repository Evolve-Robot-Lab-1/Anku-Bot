/**
 * Naval Inspection Robot - Control Panel JavaScript
 *
 * ROS2 communication via rosbridge + dual camera feeds + keyboard controls.
 * Camera feeds use JS fetch + requestAnimationFrame (proven pattern for OBSBOT).
 *
 * Movement flow: Web UI -> /cmd_vel_raw -> obstacle_avoidance -> /cmd_vel -> serial_bridge
 */

// --- Configuration ---
const CONFIG = {
    rosbridgePort: window.ROSBRIDGE_PORT || 9090,
    camServerPort: window.CAM_SERVER_PORT || 8090,
    cmdVelTopic: window.CMD_VEL_TOPIC || '/cmd_vel',  // /cmd_vel direct, /cmd_vel_raw when obstacle avoidance active
    speedTopic: '/naval/speed',
    estopTopic: '/naval/estop',
    tiltFrontTopic: '/naval/tilt_front',
    tiltRearTopic: '/naval/tilt_rear',
    statusTopic: '/naval/status',
    obstacleStatusTopic: '/naval/obstacle_status'
};

// --- State ---
let ros = null;
let cmdVelPub = null;
let speedPub = null;
let estopPub = null;
let tiltFrontPub = null;
let tiltRearPub = null;
let currentSpeed = 1;
let estopActive = false;
let strafeMode = true;  // true=strafe (lateral), false=turn (rotate)
let obstacleAvoidEnabled = false;  // false=direct /cmd_vel (default), true=use /cmd_vel_raw
let activeKeys = new Set();
let camBaseUrl = '';

// --- DOM cache ---
const el = {};

// --- Init ---
document.addEventListener('DOMContentLoaded', init);

function init() {
    cacheElements();

    const host = window.location.hostname;
    camBaseUrl = `http://${host}:${CONFIG.camServerPort}`;

    connectToROS();
    setupMovementButtons();
    setupSpeedButtons();
    setupEstop();
    setupZoomSliders();
    setupTiltSliders();
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
    el.obstacleState = document.getElementById('obstacle-state');
    el.distFront = document.getElementById('dist-front');
    el.distLeft = document.getElementById('dist-left');
    el.distRight = document.getElementById('dist-right');
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

let cmdVelDirectPub = null;

function setupPublishers() {
    cmdVelPub = new ROSLIB.Topic({
        ros: ros,
        name: CONFIG.cmdVelTopic,
        messageType: 'geometry_msgs/Twist'
    });
    cmdVelPub.advertise();

    // Direct /cmd_vel publisher (bypass obstacle avoidance)
    cmdVelDirectPub = new ROSLIB.Topic({
        ros: ros,
        name: '/cmd_vel',
        messageType: 'geometry_msgs/Twist'
    });
    cmdVelDirectPub.advertise();

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

    tiltFrontPub = new ROSLIB.Topic({
        ros: ros,
        name: CONFIG.tiltFrontTopic,
        messageType: 'std_msgs/Int32'
    });
    tiltFrontPub.advertise();

    tiltRearPub = new ROSLIB.Topic({
        ros: ros,
        name: CONFIG.tiltRearTopic,
        messageType: 'std_msgs/Int32'
    });
    tiltRearPub.advertise();
}

function setupSubscribers() {
    // Arduino status
    const statusSub = new ROSLIB.Topic({
        ros: ros,
        name: CONFIG.statusTopic,
        messageType: 'std_msgs/String'
    });
    statusSub.subscribe((msg) => {
        el.statusArduino.textContent = msg.data;
    });

    // Obstacle avoidance status
    const obstacleSub = new ROSLIB.Topic({
        ros: ros,
        name: CONFIG.obstacleStatusTopic,
        messageType: 'std_msgs/String'
    });
    obstacleSub.subscribe((msg) => {
        updateObstacleDisplay(msg.data);
    });
}


// ============================================================
// Obstacle Avoidance Display
// ============================================================

function updateObstacleDisplay(data) {
    // Parse: "state:driving,front:0.81,left:2.25,right:0.79,attempts:0"
    const parts = {};
    data.split(',').forEach(item => {
        const [key, val] = item.split(':');
        parts[key] = val;
    });

    const state = parts.state || 'unknown';
    const front = parseFloat(parts.front) || 0;
    const left = parseFloat(parts.left) || 0;
    const right = parseFloat(parts.right) || 0;

    el.distFront.textContent = front.toFixed(2);
    el.distLeft.textContent = left.toFixed(2);
    el.distRight.textContent = right.toFixed(2);

    const stateLabels = {
        'driving': 'Clear',
        'obstacle_detected': 'OBSTACLE!',
        'reversing': 'REVERSING',
        'stopped': 'STOPPED',
        'scanning': 'SCANNING',
        'turning': 'TURNING',
        'check_clear': 'CHECKING'
    };

    const stateColors = {
        'driving': '#4CAF50',
        'obstacle_detected': '#f44336',
        'reversing': '#ff9800',
        'stopped': '#ff9800',
        'scanning': '#2196F3',
        'turning': '#2196F3',
        'check_clear': '#ff9800'
    };

    el.obstacleState.textContent = stateLabels[state] || state;
    el.obstacleState.style.color = stateColors[state] || '#666';
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
    if (cmdVelPub) cmdVelPub.publish(twist);
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

function getStrafeVelocity(side) {
    // side: 1=left, -1=right
    if (strafeMode) return [0, side, 0];
    return [0, 0, side];  // turn mode: rotate instead
}

function setupMovementButtons() {
    const moves = {
        'btn-forward':      [1, 0, 0],
        'btn-backward':     [-1, 0, 0],
        'btn-rot-left':     [0, 0, 1],
        'btn-rot-right':    [0, 0, -1],
        'btn-fwd-left':     [1, 1, 0],
        'btn-fwd-right':    [1, -1, 0],
        'btn-bwd-left':     [-1, 1, 0],
        'btn-bwd-right':    [-1, -1, 0]
    };

    Object.keys(moves).forEach(id => {
        const btn = document.getElementById(id);
        if (!btn) return;
        const [lx, ly, az] = moves[id];

        btn.addEventListener('mousedown', () => sendVelocity(lx, ly, az));
        btn.addEventListener('mouseup', stopMoving);
        btn.addEventListener('mouseleave', stopMoving);

        btn.addEventListener('touchstart', (e) => {
            e.preventDefault();
            sendVelocity(lx, ly, az);
        });
        btn.addEventListener('touchend', stopMoving);
        btn.addEventListener('touchcancel', stopMoving);
    });

    // Strafe buttons: respect strafe/turn mode
    const strafeLeft = document.getElementById('btn-strafe-left');
    const strafeRight = document.getElementById('btn-strafe-right');

    if (strafeLeft) {
        strafeLeft.addEventListener('mousedown', () => { const v = getStrafeVelocity(1); sendVelocity(...v); });
        strafeLeft.addEventListener('mouseup', stopMoving);
        strafeLeft.addEventListener('mouseleave', stopMoving);
        strafeLeft.addEventListener('touchstart', (e) => { e.preventDefault(); const v = getStrafeVelocity(1); sendVelocity(...v); });
        strafeLeft.addEventListener('touchend', stopMoving);
        strafeLeft.addEventListener('touchcancel', stopMoving);
    }
    if (strafeRight) {
        strafeRight.addEventListener('mousedown', () => { const v = getStrafeVelocity(-1); sendVelocity(...v); });
        strafeRight.addEventListener('mouseup', stopMoving);
        strafeRight.addEventListener('mouseleave', stopMoving);
        strafeRight.addEventListener('touchstart', (e) => { e.preventDefault(); const v = getStrafeVelocity(-1); sendVelocity(...v); });
        strafeRight.addEventListener('touchend', stopMoving);
        strafeRight.addEventListener('touchcancel', stopMoving);
    }

    document.getElementById('btn-stop').addEventListener('click', stopMoving);
}

function toggleObstacle() {
    obstacleAvoidEnabled = !obstacleAvoidEnabled;
    const btn = document.getElementById('btn-obstacle-toggle');
    const panel = document.querySelector('.obstacle-panel');
    if (btn) {
        btn.textContent = obstacleAvoidEnabled ? 'ON' : 'OFF';
        btn.classList.toggle('obstacle-off', !obstacleAvoidEnabled);
    }
    if (panel) {
        panel.style.borderLeftColor = obstacleAvoidEnabled ? 'var(--secondary-color)' : '#666';
    }
    if (!obstacleAvoidEnabled) {
        el.obstacleState.textContent = 'DISABLED';
        el.obstacleState.style.color = '#666';
    }
    // Tell obstacle avoidance node to stop/start publishing to /cmd_vel
    fetch(`http://${window.location.hostname}:${window.location.port}/api/obstacle`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ enabled: obstacleAvoidEnabled })
    }).catch(err => console.error('Obstacle toggle error:', err));
}

function toggleStrafeMode() {
    strafeMode = !strafeMode;
    const btn = document.getElementById('btn-strafe-toggle');
    const sl = document.getElementById('btn-strafe-left');
    const sr = document.getElementById('btn-strafe-right');
    if (btn) {
        btn.textContent = strafeMode ? 'Mode: Strafe' : 'Mode: Turn';
        btn.classList.toggle('mode-turn', !strafeMode);
    }
    if (sl) sl.innerHTML = strafeMode ? '&#9664; Strafe L' : '&#8634; Turn L';
    if (sr) sr.innerHTML = strafeMode ? 'Strafe R &#9654;' : 'Turn R &#8635;';
}


// ============================================================
// Speed Control
// ============================================================

function setSpeed(level) {
    currentSpeed = level;
    // Use REST API (bypasses rosbridge Int32 issue)
    fetch(`http://${window.location.hostname}:${window.location.port}/api/speed`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ level: level })
    }).catch(err => console.error('Speed error:', err));
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
    fetch(`http://${window.location.hostname}:${window.location.port}/api/estop`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ active: estopActive })
    }).catch(err => console.error('Estop error:', err));
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
// Camera Feeds
// ============================================================

function startCameraFeeds() {
    startFeed('cam-left', `${camBaseUrl}/frame/left`, 'fps-left');
    startFeed('cam-right', `${camBaseUrl}/frame/right`, 'fps-right');
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
        .then(() => {
            el.camStatus.textContent = 'CAM: Connected';
            el.camStatus.className = 'status-indicator connected';
        })
        .catch(() => {
            el.camStatus.textContent = 'CAM: Disconnected';
            el.camStatus.className = 'status-indicator disconnected';
        });
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
        zoomLeft.addEventListener('input', (e) => { zoomLeftVal.textContent = e.target.value; });
        zoomLeft.addEventListener('change', (e) => { sendZoom('left', parseInt(e.target.value)); });
    }
    if (zoomRight) {
        zoomRight.addEventListener('input', (e) => { zoomRightVal.textContent = e.target.value; });
        zoomRight.addEventListener('change', (e) => { sendZoom('right', parseInt(e.target.value)); });
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
// Tilt Control (servo via ROS)
// ============================================================

function setupTiltSliders() {
    const tiltFront = document.getElementById('tilt-front');
    const tiltRear = document.getElementById('tilt-rear');
    const tiltFrontVal = document.getElementById('tilt-front-val');
    const tiltRearVal = document.getElementById('tilt-rear-val');

    if (tiltFront) {
        tiltFront.addEventListener('input', (e) => { tiltFrontVal.textContent = e.target.value; });
        tiltFront.addEventListener('change', (e) => {
            fetch(`http://${window.location.hostname}:${window.location.port}/api/tilt`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ side: 'front', angle: parseInt(e.target.value) })
            }).catch(err => console.error('Tilt error:', err));
        });
    }
    if (tiltRear) {
        tiltRear.addEventListener('input', (e) => { tiltRearVal.textContent = e.target.value; });
        tiltRear.addEventListener('change', (e) => {
            fetch(`http://${window.location.hostname}:${window.location.port}/api/tilt`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ side: 'rear', angle: parseInt(e.target.value) })
            }).catch(err => console.error('Tilt error:', err));
        });
    }
}


// ============================================================
// Keyboard Controls
// ============================================================

function setupKeyboard() {
    document.addEventListener('keydown', handleKeyDown);
    document.addEventListener('keyup', handleKeyUp);
}

function handleKeyDown(event) {
    if (event.target.tagName === 'INPUT' || event.target.tagName === 'TEXTAREA') return;
    if (event.repeat) return;

    const key = event.key.toLowerCase();
    activeKeys.add(key);

    switch (key) {
        case 'w': sendVelocity(1, 0, 0); break;
        case 's': sendVelocity(-1, 0, 0); break;
        case 'a': strafeMode ? sendVelocity(0, 1, 0) : sendVelocity(0, 0, 1); break;
        case 'd': strafeMode ? sendVelocity(0, -1, 0) : sendVelocity(0, 0, -1); break;
        case 'q': sendVelocity(0, 0, 1); break;
        case 'e': sendVelocity(0, 0, -1); break;
        case '1': setSpeed(1); break;
        case '2': setSpeed(2); break;
        case '3': setSpeed(3); break;
        case ' ':
            event.preventDefault();
            stopMoving();
            break;
        case 'm':
            toggleStrafeMode();
            break;
        case 'o':
            toggleObstacle();
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
        const moveKeys = ['w', 's', 'a', 'd', 'q', 'e'];
        const anyHeld = moveKeys.some(k => activeKeys.has(k));
        if (!anyHeld) {
            stopMoving();
        }
    }
}
