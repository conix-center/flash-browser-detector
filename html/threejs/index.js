var codes = [0b10101010, 0b10011010, 0b10100011];
var targetFps = 30;

var stats = null;

var scene, camera, renderer;

const dtagMatrix = new THREE.Matrix4();
const FLIPMATRIX = new THREE.Matrix4();
FLIPMATRIX.set(
    1, 0, 0, 0,
    0, -1, 0, 0,
    0, 0, -1, 0,
    0, 0, 0, 1,
);

const originMatrix = new THREE.Matrix4();
originMatrix.set(
    1,  0, 0, 0,
    0,  0, 1, 0,
    0, -1, 0, 0,
    0,  0, 0, 1,
);

var flashSource = new Flash.FlashSource();
flashSource.setOptions({
    // width: 4032,
    // height: 3024,
    // width: 1920,
    // height: 1080,
    width: window.innerWidth,
    height: window.innerHeight,
});

var overlayCanvas = document.createElement("canvas");
overlayCanvas.id = "overlay";
overlayCanvas.style.position = "absolute";
overlayCanvas.style.top = "0px";
overlayCanvas.style.left = "0px";
overlayCanvas.width = window.innerWidth;
overlayCanvas.height = window.innerHeight;

var flashDetector = new Flash.FlashDetector(codes, targetFps, flashSource);

function initDemo() {
    flashSource.init()
        .then((source) => {
            document.body.appendChild(source);
            document.body.appendChild(overlayCanvas);

            stats = new Stats();
            stats.showPanel(0);
            document.getElementById("stats").appendChild(stats.domElement);

            resize();
            updateInfo();
            initFlash();
        })
        .catch((err) => {
            console.error(err);
        });
}

async function initFlash() {
    await flashDetector.init();

    const timer = flashDetector.createTimer(tick);
    timer.run();

    let ratio = flashSource.options.width / flashSource.options.height;

    scene = new THREE.Scene();
    camera = new THREE.PerspectiveCamera(75, ratio, 0.01, 1000);

    renderer = new THREE.WebGLRenderer({
        canvas: overlayCanvas,
        alpha: true,
        antialias: true,
        precision: "mediump",
        premultipliedAlpha: true,
        stencil: true,
        depth: true,
        logarithmicDepthBuffer: true,
        objVisibility: false
    });
    renderer.setSize(window.innerWidth, window.innerHeight);
    renderer.setPixelRatio(window.devicePixelRatio);

    const light = new THREE.AmbientLight(0xffffff)
    scene.add(light);

    // const geometry = new THREE.BoxGeometry( 1, 1, 1 );
    // const material = new THREE.MeshBasicMaterial( {color: 0x00ff00} );
    // const cube = new THREE.Mesh( geometry, material );
    const geometry = new THREE.BoxGeometry( 1, 1, 1 );
    const edges = new THREE.EdgesGeometry( geometry );
    const material = new THREE.LineBasicMaterial( { color: 0x00ff00 } );
    const cube = new THREE.LineSegments( edges, material );

    camera.position.y = 5;
    camera.position.z = 5;

    let rotMat = new THREE.Matrix4();
    rotMat.set(
        1, 0, 0, 0,
        0, 0.707, 0.707, 0,
        0, -0.707, 0.707, 0,
        0, 0, 0, 1,
    );
    camera.quaternion.setFromRotationMatrix(rotMat);

    let root = new THREE.Object3D();
    root.add(cube);

    scene.add(root);

    var renderLoop = function () {
        renderer.render(scene, camera);
        requestAnimationFrame(renderLoop);
    };
    renderLoop();
}


function tick() {
    stats.update();
    flashSource.getPixels().then((imageData) => {
        flashDetector.detectTags(imageData);
    });
}

function updateInfo() {
    var info = document.getElementById("info");
    info.style.zIndex = "1";
    info.innerText = "Detecting Codes:\n";
    for (code of codes) {
        info.innerText += `${Flash.Utils.dec2bin(code)} (${code})\n`;
    }
}

function drawTags(tags) {
    for (tag of tags) {
        const rigPose = getPose(tag.pose.R, tag.pose.t);
        camera.quaternion.setFromRotationMatrix(rigPose);
        camera.position.setFromMatrixPosition(rigPose);
    }
}

function getPose(r, t) {
    dtagMatrix.set(
        r[0], r[3], r[6], t[0],
        r[1], r[4], r[7], t[1],
        r[2], r[5], r[8], t[2],
        0   , 0   , 0   , 1
    );

    dtagMatrix.premultiply(FLIPMATRIX);
    dtagMatrix.multiply(FLIPMATRIX);

    var res = new THREE.Matrix4();
    dtagMatrix.copy(dtagMatrix).invert();
    res.identity();
    res.multiplyMatrices(originMatrix, dtagMatrix);

    return res;
}

window.addEventListener("onFlashTagsFound", (e) => {
    const tags = e.detail.tags;
    drawTags(tags);
});

function resize() {
    flashSource.resize(window.innerWidth, window.innerHeight);
    flashSource.copyDimensionsTo(overlayCanvas);
}

window.addEventListener("resize", (e) => {
    resize();
});

initDemo();
