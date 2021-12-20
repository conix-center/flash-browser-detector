var codes = [0b10101010, 0b10011010, 0b10100011];
var targetFps = 30;

var stats = null;

const rigMatrix = new THREE.Matrix4();
const dtagMatrix = new THREE.Matrix4();
const vioMatrix = new THREE.Matrix4();
const vioMatrixInv = new THREE.Matrix4();

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

var flashDetector = new Flash.FlashDetector(codes, targetFps, flashSource);
flashDetector.setOptions({
    // printPerformance: true,
});
flashDetector.init();

function updateInfo() {
    var info = document.getElementById("info");
    info.style.zIndex = "1";
    info.innerText = "Detecting Codes:\n";
    for (code of this.codes) {
        info.innerText += `${Flash.Utils.dec2bin(code)} (${code})\n`;
    }
}

function drawTags(tags) {
    for (tag of tags) {
        if (tag.code != codes[0]) continue;

        const cameraElem = document.getElementById("my-camera");
        const camParent = cameraElem.object3D.parent.matrixWorld;
        const cam = cameraElem.object3D.matrixWorld;
        vioMatrix.copy(camParent).invert();
        vioMatrix.multiply(cam);
        vioMatrixInv.copy(vioMatrix).invert();

        const rigPose = getPose(tag.pose.R, tag.pose.t);
        document.getElementById('cameraSpinner').object3D.quaternion.setFromRotationMatrix(rigPose);
        document.getElementById('cameraRig').object3D.position.setFromMatrixPosition(rigPose);
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

    dtagMatrix.copy(dtagMatrix).invert();
    rigMatrix.identity();
    rigMatrix.multiplyMatrices(originMatrix, dtagMatrix);
    rigMatrix.multiply(vioMatrixInv);

    return rigMatrix;
}

window.addEventListener("onFlashInit", (e) => {
    stats = new Stats();
    stats.showPanel(0);
    document.getElementById("stats").appendChild(stats.domElement);

    document.body.appendChild(e.detail.source);
    // document.body.appendChild(flashDetector.preprocessor.canvas);

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
    // flashSource.copyDimensionsTo(overlayCanvas);
}

window.addEventListener("resize", (e) => {
    resize();
});

AFRAME.registerComponent('wireframe', {
    dependencies: ['material'],
    init: function () {
      this.el.components.material.material.wireframe = true;
    }
});
