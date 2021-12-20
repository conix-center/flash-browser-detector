var codes = [0b10101010, 0b10011010, 0b10100011];
var targetFps = 30;

var stats = null;

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

function drawTag(tag) {
    var overlayCtx = overlayCanvas.getContext("2d");

    overlayCtx.beginPath();
        overlayCtx.lineWidth = 3;
        overlayCtx.strokeStyle = "blue";
        overlayCtx.moveTo(tag.corners[0].x, tag.corners[0].y);
        overlayCtx.lineTo(tag.corners[1].x, tag.corners[1].y);
        overlayCtx.lineTo(tag.corners[2].x, tag.corners[2].y);
        overlayCtx.lineTo(tag.corners[3].x, tag.corners[3].y);
        overlayCtx.lineTo(tag.corners[0].x, tag.corners[0].y);
    overlayCtx.stroke();

    overlayCtx.font = "bold 20px Arial";
    overlayCtx.textAlign = "center";
    overlayCtx.fillStyle = "red";
    overlayCtx.fillText(tag.code, tag.center.x, tag.center.y);
}

function drawTags(tags) {
    var overlayCtx = overlayCanvas.getContext("2d");
    overlayCtx.clearRect(0, 0, overlayCanvas.width, overlayCanvas.height);

    for (tag of tags) {
        drawTag(tag);
    }
}

window.addEventListener("onFlashInit", (e) => {
    stats = new Stats();
    stats.showPanel(0);
    document.getElementById("stats").appendChild(stats.domElement);

    document.body.appendChild(e.detail.source);
    document.body.appendChild(overlayCanvas);
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
    flashSource.copyDimensionsTo(overlayCanvas);
}

window.addEventListener("resize", (e) => {
    resize();
});
