let width = window.innerWidth;
let height = window.innerHeight;
if (window.innerWidth > 1000) {
    width /= 2;
    height /= 2;
}

const code = 0xaf;
const targetFps = 30;

let stats = null;
let videoSource = null;
let videoCanvas = null;
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
        overlayCtx.fillText(dec2bin(code), quad.center.x, quad.center.y+10);
    overlayCtx.stroke();
}

function drawQuads(quads) {
    const overlayCtx = overlayCanvas.getContext("2d");
    overlayCtx.clearRect(0, 0, width, height);

    for (var i = 0; i < quads.length; i++) {
        drawQuad(quads[i]);
    }
}

document.addEventListener("onGlitterInit", (e) => {
    stats = new Stats();
    stats.showPanel(0);
    document.getElementById("stats").appendChild(stats.domElement);
    videoSource = e.detail.source;
});

document.addEventListener("onGlitterTagsFound", (e) => {
    const videoCanvasCtx = videoCanvas.getContext("2d");
    videoCanvasCtx.drawImage(
        videoSource, 0, 0, width, height);
    drawQuads(e.detail.tags);
    stats.update();
});

function setVideoStyle(elem) {
    elem.style.position = "absolute";
    elem.style.top = 0;
}

window.onload = () => {
    var video = document.createElement("video");
    video.setAttribute("autoplay", "");
    video.setAttribute("muted", "");
    video.setAttribute("playsinline", "");

    videoCanvas = document.createElement("canvas");
    setVideoStyle(videoCanvas);
    videoCanvas.id = "video-canvas";
    videoCanvas.width = width;
    videoCanvas.height = height;
    videoCanvas.style.zIndex = 0;
    document.body.appendChild(videoCanvas);

    overlayCanvas = document.createElement("canvas");
    setVideoStyle(overlayCanvas);
    overlayCanvas.id = "overlay";
    overlayCanvas.width = width;
    overlayCanvas.height = height;
    overlayCanvas.style.zIndex = 1;
    document.body.appendChild(overlayCanvas);

    glitterDetector = new Glitter.GlitterDetector(code, targetFps, width, height, video);
    // glitterDetector.printPerformance = true;
    glitterDetector.start();
}
