let width = window.innerWidth;
let height = window.innerHeight;

const code = 0xaf;
const targetFps = 30;

let stats = null;
let info = null;
let videoSource = null;
let overlayCanvas = null;

let glitterDetector = null;

function dec2bin(dec) {
    return (dec >>> 0).toString(2);
}

function drawQuad(quad) {
    const overlayCtx = overlayCanvas.getContext("2d");

    overlayCtx.beginPath();
        overlayCtx.lineWidth = 5;
        overlayCtx.strokeStyle = "blue";
        overlayCtx.moveTo(quad.corners[0].x, quad.corners[0].y);
        overlayCtx.lineTo(quad.corners[1].x, quad.corners[1].y);
        overlayCtx.lineTo(quad.corners[2].x, quad.corners[2].y);
        overlayCtx.lineTo(quad.corners[3].x, quad.corners[3].y);
        overlayCtx.lineTo(quad.corners[0].x, quad.corners[0].y);

        overlayCtx.font = "bold 20px Arial";
        overlayCtx.textAlign = "center";
        overlayCtx.fillStyle = "blue";
        overlayCtx.fillText(dec2bin(code), quad.center.x, quad.center.y);
    overlayCtx.stroke();
}

function drawQuads(quads) {
    const overlayCtx = overlayCanvas.getContext("2d");
    overlayCtx.clearRect(0, 0, overlayCanvas.width, overlayCanvas.height);

    for (var i = 0; i < quads.length; i++) {
        drawQuad(quads[i]);
    }
}

window.addEventListener("onGlitterInit", (e) => {
    stats = new Stats();
    stats.showPanel(0);
    document.getElementById("stats").appendChild(stats.domElement);
    videoSource = e.detail.source;
    document.body.appendChild(videoSource);
    resize(width, height);
});

window.addEventListener("onGlitterTagsFound", (e) => {
    drawQuads(e.detail.tags);
    stats.update();
});

window.addEventListener("onGlitterCalibrate", (e) => {
    info.innerText = `Detecting Code:\n${dec2bin(code)} (${code})\n` + e.detail.decimationFactor;
});

function resize(newWidth, newHeight) {
    var screenWidth = newWidth;
    var screenHeight = newHeight;

    var sourceWidth = videoSource.videoWidth;
    var sourceHeight = videoSource.videoHeight;

    var sourceAspect = sourceWidth / sourceHeight;
    var screenAspect = screenWidth / screenHeight;

    if (screenAspect < sourceAspect) {
        // compute newWidth and set .width/.marginLeft
        var newWidth = sourceAspect * screenHeight;
        videoSource.style.width = newWidth + "px";
        videoSource.style.marginLeft = -(newWidth - screenWidth) / 2 + "px";

        // init style.height/.marginTop to normal value
        videoSource.style.height = screenHeight + "px";
        videoSource.style.marginTop = "0px";
    } else {
        // compute newHeight and set .height/.marginTop
        var newHeight = 1 / (sourceAspect / screenWidth);
        videoSource.style.height = newHeight + "px";
        videoSource.style.marginTop = -(newHeight - screenHeight) / 2 + "px";

        // init style.width/.marginLeft to normal value
        videoSource.style.width = screenWidth + "px";
        videoSource.style.marginLeft = "0px";
    }

    overlayCanvas.style.width = videoSource.style.width
    overlayCanvas.style.height = videoSource.style.height
    overlayCanvas.style.marginLeft = videoSource.style.marginLeft
    overlayCanvas.style.marginTop = videoSource.style.marginTop
}

window.addEventListener("resize", (e) => {
    resize(window.innerWidth, window.innerHeight);
});

function setVideoStyle(elem) {
    elem.style.position = "absolute";
    elem.style.top = "0px";
    elem.style.left = "0px";
    elem.width = 640;
    elem.height = 480;
}

window.onload = () => {
    overlayCanvas = document.createElement("canvas");
    overlayCanvas.id = "overlay";
    setVideoStyle(overlayCanvas);
    document.body.appendChild(overlayCanvas);

    info = document.getElementById("info");
    info.innerText = `Detecting Code:\n${dec2bin(code)} (${code})`;
    info.style.zIndex = "1";

    glitterDetector = new Glitter.GlitterDetector(code, targetFps, width, height);
    // glitterDetector.setOptions({
    //     printPerformance: true,
    // });
    glitterDetector.start();
}
