/**
 * Robot Control Panel - JavaScript Control Logic
 * Handles ROS2 communication via rosbridge and UI interactions
 */

// Configuration
const CONFIG = {
    rosbridgePort: window.ROSBRIDGE_PORT || 9090,
    videoServerPort: window.VIDEO_SERVER_PORT || 8080,
    cameraTopic: '/camera/image_raw',
    cmdVelTopic: '/cmd_vel',
    cmdVelRawTopic: '/cmd_vel_raw',
    armCommandTopic: '/arm/joint_commands',
    armStateTopic: '/arm/joint_states',
    scanTopic: '/scan',
    mapTopic: '/map',
    amclPoseTopic: '/amcl_pose',
    navGoalAction: '/navigate_to_pose'
};

// Arm pose definitions
const ARM_POSES = {
    home: [0, 0, 0, 0, 0, 0],
    search: [0, -0.5, 0.3, 0, 0, 0],
    prePick: [0, -0.8, 0.6, 0, 0, 0],
    pick: [0, -1.0, 0.8, 0, 0, 0],
    carry: [0, -0.3, 0.2, 0, 0, 0],
    prePlace: [1.57, -0.8, 0.6, 0, 0, 0],
    place: [1.57, -1.0, 0.8, 0, 0, 0]
};

const GRIPPER = {
    open: 0.5,
    closed: 0.0
};

const JOINT_NAMES = ['joint_1', 'joint_2', 'joint_3', 'joint_4', 'gripper_base_joint', 'left_gear_joint'];

// State
let ros = null;
let cmdVelPublisher = null;
let cmdVelRawPublisher = null;
let armCommandPublisher = null;
let navGoalClient = null;
let currentMode = 'manual';
let currentSpeed = 0.3;
let angularSpeed = 0.5;
let isMoving = false;
let eStopActive = false;
let mapData = null;
let robotPose = { x: 0, y: 0, theta: 0 };

// DOM Elements
let elements = {};

// Initialize when DOM is ready
document.addEventListener('DOMContentLoaded', init);

function init() {
    // Cache DOM elements
    cacheElements();

    // Setup camera feed
    setupCameraFeed();

    // Connect to ROS
    connectToROS();

    // Setup UI event listeners
    setupEventListeners();

    // Setup map canvas
    setupMapCanvas();
}

function cacheElements() {
    elements = {
        rosStatus: document.getElementById('ros-status'),
        cameraFeed: document.getElementById('camera-feed'),
        navMap: document.getElementById('nav-map'),
        speedSlider: document.getElementById('speed-slider'),
        speedValue: document.getElementById('speed-value'),
        statusMode: document.getElementById('status-mode'),
        statusArm: document.getElementById('status-arm'),
        statusObstacle: document.getElementById('status-obstacle'),
        statusNav: document.getElementById('status-nav'),
        statusLinearVel: document.getElementById('status-linear-vel'),
        statusAngularVel: document.getElementById('status-angular-vel'),
        btnCancelNav: document.getElementById('btn-cancel-nav')
    };
}

function setupCameraFeed() {
    const host = window.location.hostname;
    const videoUrl = `http://${host}:${CONFIG.videoServerPort}/stream?topic=${CONFIG.cameraTopic}&type=mjpeg&quality=50`;
    elements.cameraFeed.src = videoUrl;
}

function connectToROS() {
    const host = window.location.hostname;
    const url = `ws://${host}:${CONFIG.rosbridgePort}`;

    ros = new ROSLIB.Ros({ url: url });

    ros.on('connection', () => {
        console.log('Connected to ROS');
        updateConnectionStatus(true);
        setupPublishers();
        setupSubscribers();
        setupActionClients();
    });

    ros.on('error', (error) => {
        console.error('ROS connection error:', error);
        updateConnectionStatus(false);
    });

    ros.on('close', () => {
        console.log('ROS connection closed');
        updateConnectionStatus(false);
        // Attempt reconnection after 3 seconds
        setTimeout(connectToROS, 3000);
    });
}

function updateConnectionStatus(connected) {
    elements.rosStatus.textContent = connected ? 'ROS: Connected' : 'ROS: Disconnected';
    elements.rosStatus.className = 'status-indicator ' + (connected ? 'connected' : 'disconnected');
}

function setupPublishers() {
    // cmd_vel publisher (for manual mode)
    cmdVelPublisher = new ROSLIB.Topic({
        ros: ros,
        name: CONFIG.cmdVelTopic,
        messageType: 'geometry_msgs/Twist'
    });

    // cmd_vel_raw publisher (for auto mode with obstacle avoidance)
    cmdVelRawPublisher = new ROSLIB.Topic({
        ros: ros,
        name: CONFIG.cmdVelRawTopic,
        messageType: 'geometry_msgs/Twist'
    });

    // Arm command publisher
    armCommandPublisher = new ROSLIB.Topic({
        ros: ros,
        name: CONFIG.armCommandTopic,
        messageType: 'sensor_msgs/JointState'
    });
}

function setupSubscribers() {
    // Subscribe to arm state
    const armStateSub = new ROSLIB.Topic({
        ros: ros,
        name: CONFIG.armStateTopic,
        messageType: 'sensor_msgs/JointState'
    });
    armStateSub.subscribe((msg) => {
        elements.statusArm.textContent = 'Active';
    });

    // Subscribe to laser scan for obstacle detection display
    const scanSub = new ROSLIB.Topic({
        ros: ros,
        name: CONFIG.scanTopic,
        messageType: 'sensor_msgs/LaserScan'
    });
    scanSub.subscribe((msg) => {
        // Check for obstacles in front
        const frontAngles = msg.ranges.slice(msg.ranges.length/2 - 30, msg.ranges.length/2 + 30);
        const minDist = Math.min(...frontAngles.filter(r => r > msg.range_min && r < msg.range_max));
        elements.statusObstacle.textContent = minDist < 0.3 ? 'OBSTACLE!' : 'Clear';
        elements.statusObstacle.style.color = minDist < 0.3 ? '#f44336' : '#4CAF50';
    });

    // Subscribe to map
    const mapSub = new ROSLIB.Topic({
        ros: ros,
        name: CONFIG.mapTopic,
        messageType: 'nav_msgs/OccupancyGrid'
    });
    mapSub.subscribe((msg) => {
        mapData = msg;
        drawMap();
    });

    // Subscribe to robot pose (AMCL)
    const poseSub = new ROSLIB.Topic({
        ros: ros,
        name: CONFIG.amclPoseTopic,
        messageType: 'geometry_msgs/PoseWithCovarianceStamped'
    });
    poseSub.subscribe((msg) => {
        robotPose.x = msg.pose.pose.position.x;
        robotPose.y = msg.pose.pose.position.y;
        // Extract yaw from quaternion
        const q = msg.pose.pose.orientation;
        robotPose.theta = Math.atan2(2 * (q.w * q.z + q.x * q.y), 1 - 2 * (q.y * q.y + q.z * q.z));
        drawMap();
    });
}

function setupActionClients() {
    // Nav2 action client
    navGoalClient = new ROSLIB.ActionClient({
        ros: ros,
        serverName: CONFIG.navGoalAction,
        actionName: 'nav2_msgs/NavigateToPose'
    });
}

function setupEventListeners() {
    // Mode buttons
    document.getElementById('btn-manual').addEventListener('click', () => setMode('manual'));
    document.getElementById('btn-auto').addEventListener('click', () => setMode('auto'));
    document.getElementById('btn-nav').addEventListener('click', () => setMode('nav'));

    // Movement buttons
    const btnForward = document.getElementById('btn-forward');
    const btnBackward = document.getElementById('btn-backward');
    const btnLeft = document.getElementById('btn-left');
    const btnRight = document.getElementById('btn-right');
    const btnStop = document.getElementById('btn-stop');

    // Mouse events
    btnForward.addEventListener('mousedown', () => startMoving(1, 0));
    btnBackward.addEventListener('mousedown', () => startMoving(-1, 0));
    btnLeft.addEventListener('mousedown', () => startMoving(0, 1));
    btnRight.addEventListener('mousedown', () => startMoving(0, -1));

    [btnForward, btnBackward, btnLeft, btnRight].forEach(btn => {
        btn.addEventListener('mouseup', stopMoving);
        btn.addEventListener('mouseleave', stopMoving);
    });

    // Touch events for mobile
    btnForward.addEventListener('touchstart', (e) => { e.preventDefault(); startMoving(1, 0); });
    btnBackward.addEventListener('touchstart', (e) => { e.preventDefault(); startMoving(-1, 0); });
    btnLeft.addEventListener('touchstart', (e) => { e.preventDefault(); startMoving(0, 1); });
    btnRight.addEventListener('touchstart', (e) => { e.preventDefault(); startMoving(0, -1); });

    [btnForward, btnBackward, btnLeft, btnRight].forEach(btn => {
        btn.addEventListener('touchend', stopMoving);
        btn.addEventListener('touchcancel', stopMoving);
    });

    btnStop.addEventListener('click', stopMoving);

    // E-Stop
    document.getElementById('btn-estop').addEventListener('click', toggleEStop);

    // Speed slider
    elements.speedSlider.addEventListener('input', (e) => {
        currentSpeed = parseFloat(e.target.value);
        elements.speedValue.textContent = currentSpeed.toFixed(1);
    });

    // Arm pose buttons
    document.getElementById('btn-home').addEventListener('click', () => sendArmPose('home'));
    document.getElementById('btn-search').addEventListener('click', () => sendArmPose('search'));
    document.getElementById('btn-pick').addEventListener('click', () => sendArmPose('pick'));
    document.getElementById('btn-place').addEventListener('click', () => sendArmPose('place'));
    document.getElementById('btn-carry').addEventListener('click', () => sendArmPose('carry'));

    // Gripper buttons
    document.getElementById('btn-gripper-open').addEventListener('click', () => sendGripper(GRIPPER.open));
    document.getElementById('btn-gripper-close').addEventListener('click', () => sendGripper(GRIPPER.closed));

    // Cancel navigation
    elements.btnCancelNav.addEventListener('click', cancelNavigation);

    // Keyboard controls
    document.addEventListener('keydown', handleKeyDown);
    document.addEventListener('keyup', handleKeyUp);
}

function setMode(mode) {
    currentMode = mode;

    // Update UI
    document.querySelectorAll('.btn-mode').forEach(btn => btn.classList.remove('active'));
    document.getElementById('btn-' + mode).classList.add('active');
    elements.statusMode.textContent = mode.charAt(0).toUpperCase() + mode.slice(1);

    // Stop any current movement when changing modes
    stopMoving();
}

function startMoving(linear, angular) {
    if (eStopActive) return;

    isMoving = true;
    sendVelocity(linear * currentSpeed, angular * angularSpeed);

    // Update status display
    elements.statusLinearVel.textContent = (linear * currentSpeed).toFixed(2);
    elements.statusAngularVel.textContent = (angular * angularSpeed).toFixed(2);
}

function stopMoving() {
    isMoving = false;
    sendVelocity(0, 0);

    elements.statusLinearVel.textContent = '0.00';
    elements.statusAngularVel.textContent = '0.00';
}

function sendVelocity(linear, angular) {
    const twist = new ROSLIB.Message({
        linear: { x: linear, y: 0, z: 0 },
        angular: { x: 0, y: 0, z: angular }
    });

    // Choose publisher based on mode
    const publisher = (currentMode === 'auto') ? cmdVelRawPublisher : cmdVelPublisher;

    if (publisher) {
        publisher.publish(twist);
    }
}

function toggleEStop() {
    eStopActive = !eStopActive;
    const btn = document.getElementById('btn-estop');

    if (eStopActive) {
        stopMoving();
        btn.textContent = 'RELEASE E-STOP';
        btn.classList.add('btn-estop-active');
    } else {
        btn.textContent = 'E-STOP';
        btn.classList.remove('btn-estop-active');
    }
}

function sendArmPose(poseName) {
    if (!armCommandPublisher) return;

    const pose = ARM_POSES[poseName];
    if (!pose) return;

    const msg = new ROSLIB.Message({
        header: {
            stamp: { sec: 0, nanosec: 0 },
            frame_id: ''
        },
        name: JOINT_NAMES,
        position: pose,
        velocity: [],
        effort: []
    });

    armCommandPublisher.publish(msg);
    elements.statusArm.textContent = poseName.charAt(0).toUpperCase() + poseName.slice(1);
}

function sendGripper(position) {
    if (!armCommandPublisher) return;

    const msg = new ROSLIB.Message({
        header: {
            stamp: { sec: 0, nanosec: 0 },
            frame_id: ''
        },
        name: ['gripper_base_joint'],
        position: [position],
        velocity: [],
        effort: []
    });

    armCommandPublisher.publish(msg);
    elements.statusArm.textContent = position > 0.2 ? 'Gripper Open' : 'Gripper Closed';
}

function setupMapCanvas() {
    const canvas = elements.navMap;
    canvas.addEventListener('click', handleMapClick);
}

function drawMap() {
    const canvas = elements.navMap;
    const ctx = canvas.getContext('2d');

    if (!mapData) {
        // Draw empty map
        ctx.fillStyle = '#2a2a4a';
        ctx.fillRect(0, 0, canvas.width, canvas.height);
        ctx.fillStyle = '#666';
        ctx.font = '14px Arial';
        ctx.textAlign = 'center';
        ctx.fillText('No map data', canvas.width/2, canvas.height/2);
        return;
    }

    const info = mapData.info;
    const data = mapData.data;
    const width = info.width;
    const height = info.height;
    const resolution = info.resolution;

    // Scale to canvas
    const scaleX = canvas.width / width;
    const scaleY = canvas.height / height;

    // Draw map
    const imageData = ctx.createImageData(canvas.width, canvas.height);

    for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x++) {
            const mapIdx = y * width + x;
            const val = data[mapIdx];

            const canvasX = Math.floor(x * scaleX);
            const canvasY = Math.floor((height - y - 1) * scaleY);  // Flip Y
            const canvasIdx = (canvasY * canvas.width + canvasX) * 4;

            let color;
            if (val === -1) {
                color = [100, 100, 100];  // Unknown - gray
            } else if (val === 0) {
                color = [255, 255, 255];  // Free - white
            } else {
                color = [0, 0, 0];  // Occupied - black
            }

            imageData.data[canvasIdx] = color[0];
            imageData.data[canvasIdx + 1] = color[1];
            imageData.data[canvasIdx + 2] = color[2];
            imageData.data[canvasIdx + 3] = 255;
        }
    }

    ctx.putImageData(imageData, 0, 0);

    // Draw robot position
    const robotCanvasX = (robotPose.x - info.origin.position.x) / resolution * scaleX;
    const robotCanvasY = canvas.height - (robotPose.y - info.origin.position.y) / resolution * scaleY;

    ctx.fillStyle = '#2196F3';
    ctx.beginPath();
    ctx.arc(robotCanvasX, robotCanvasY, 8, 0, 2 * Math.PI);
    ctx.fill();

    // Draw robot heading
    ctx.strokeStyle = '#2196F3';
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.moveTo(robotCanvasX, robotCanvasY);
    ctx.lineTo(
        robotCanvasX + Math.cos(-robotPose.theta) * 15,
        robotCanvasY + Math.sin(-robotPose.theta) * 15
    );
    ctx.stroke();
}

function handleMapClick(event) {
    if (currentMode !== 'nav' || !mapData) return;

    const canvas = elements.navMap;
    const rect = canvas.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const y = event.clientY - rect.top;

    const info = mapData.info;
    const scaleX = canvas.width / info.width;
    const scaleY = canvas.height / info.height;

    // Convert canvas coordinates to map coordinates
    const mapX = (x / scaleX) * info.resolution + info.origin.position.x;
    const mapY = ((canvas.height - y) / scaleY) * info.resolution + info.origin.position.y;

    sendNavGoal(mapX, mapY, 0);
}

function sendNavGoal(x, y, theta) {
    if (!navGoalClient) return;

    const goal = new ROSLIB.Goal({
        actionClient: navGoalClient,
        goalMessage: {
            pose: {
                header: {
                    stamp: { sec: 0, nanosec: 0 },
                    frame_id: 'map'
                },
                pose: {
                    position: { x: x, y: y, z: 0 },
                    orientation: {
                        x: 0,
                        y: 0,
                        z: Math.sin(theta / 2),
                        w: Math.cos(theta / 2)
                    }
                }
            }
        }
    });

    goal.on('feedback', (feedback) => {
        elements.statusNav.textContent = 'Navigating...';
    });

    goal.on('result', (result) => {
        elements.statusNav.textContent = 'Completed';
        elements.btnCancelNav.disabled = true;
    });

    goal.send();
    elements.statusNav.textContent = 'Goal sent';
    elements.btnCancelNav.disabled = false;
}

function cancelNavigation() {
    if (navGoalClient) {
        // Cancel all goals
        navGoalClient.cancel();
        elements.statusNav.textContent = 'Cancelled';
        elements.btnCancelNav.disabled = true;
    }
}

function handleKeyDown(event) {
    if (event.repeat) return;

    switch(event.key.toLowerCase()) {
        case 'w': startMoving(1, 0); break;
        case 's': startMoving(-1, 0); break;
        case 'a': startMoving(0, 1); break;
        case 'd': startMoving(0, -1); break;
        case ' ': stopMoving(); break;
        case 'escape': toggleEStop(); break;
    }
}

function handleKeyUp(event) {
    switch(event.key.toLowerCase()) {
        case 'w':
        case 's':
        case 'a':
        case 'd':
            stopMoving();
            break;
    }
}
