var codes = [0b10101010, 0b10011010, 0b10100011];
var targetFps = 30;

var stats = null;

var scene, camera, renderer, root;

var dtagMatrix = new THREE.Matrix4();
var FLIPMATRIX = new THREE.Matrix4();
FLIPMATRIX.set(
    1, 0, 0, 0,
    0, -1, 0, 0,
    0, 0, -1, 0,
    0, 0, 0, 1,
);

var flashSource = new Flash.FlashSource();
flashSource.setOptions({
    // width: 4032,
    // height: 3024,
    // width: 1920,
    // height: 1080,
    width: 1280,
    height: 720,
});

var overlayCanvas = document.createElement("canvas");
overlayCanvas.id = "overlay";
overlayCanvas.style.position = "absolute";
overlayCanvas.style.top = "0px";
overlayCanvas.style.left = "0px";
overlayCanvas.width = flashSource.options.width;
overlayCanvas.height = flashSource.options.height;

var flashDetector = new Flash.FlashDetector(codes, targetFps, flashSource);
flashDetector.setOptions({
    // printPerformance: true,
    decimateImage: false,
});
flashDetector.init();

function setMatrix(matrix, value) {
    const array = [];
    for (const key in value) {
      array[key] = value[key]
    }
    if (typeof matrix.elements.set === 'function') {
      matrix.elements.set(array)
    } else {
      matrix.elements = [].slice.call(array)
    }
};

function drawTags(tags) {
    if (!root) return;
    for (tag of tags) {
        const mat = getPose(tag.R, tag.T);
        setMatrix(root.matrix, mat);
    }
}

function updateInfo() {
    var info = document.getElementById("info");
    info.style.zIndex = "1";
    info.innerText = "Detecting Codes:\n";
    for (code of this.codes) {
        info.innerText += `${Flash.Utils.dec2bin(code)} (${code})\n`;
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

    return dtagMatrix.elements;
}

window.addEventListener("onFlashInit", (e) => {
    stats = new Stats();
    stats.showPanel(0);
    document.getElementById("stats").appendChild(stats.domElement);

    document.body.appendChild(e.detail.source);
    document.body.appendChild(overlayCanvas);
    // document.body.appendChild(flashDetector.preprocessor.canvas);

    let fov = 0.8 * 180 / Math.PI;
    let ratio = window.innerWidth / window.innerHeight;

    scene = new THREE.Scene();
    camera = new THREE.PerspectiveCamera(fov, ratio, 0.01, 1000);

    renderer = new THREE.WebGLRenderer({
        canvas: document.getElementById("overlay"),
        alpha: true,
        antialias: true,
        context: null,
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

    var geometry = new THREE.BoxGeometry(1, 1, 1);
    var material = new THREE.MeshLambertMaterial({color: 0x0000aa});
    var cube = new THREE.Mesh(geometry, material);

    root = new THREE.Object3D();
    root.matrixAutoUpdate = false;
    root.add(cube);

    scene.add(root);

    var tick = function () {
        renderer.render(scene, camera);
        requestAnimationFrame(tick);
    };
    tick();

    updateInfo();
    resize();
});

window.addEventListener("onFlashTagsFound", (e) => {
    const tags = e.detail.tags;
    drawTags(tags);
    stats.update();
});

window.addEventListener("onFlashCalibrate", (e) => {
    updateInfo();
    info.innerText += e.detail.decimationFactor;
});

function resize() {
    flashSource.resize(window.innerWidth, window.innerHeight);
    flashSource.copyDimensionsTo(overlayCanvas);
}

window.addEventListener("resize", (e) => {
    resize();
});
