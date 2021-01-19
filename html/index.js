let width = window.innerWidth;
let height = window.innerHeight;

const code = 0xaf;
const targetFps = 30;

let stats = null;
let videoSource = null;
let videoCanvas = null;
let overlayCanvas = null;

let glitterDetector = null;

function drawQuad(quad) {
    const overlayCtx = overlayCanvas.getContext("2d");

    overlayCtx.beginPath();
    overlayCtx.strokeStyle = "blue";
    overlayCtx.lineWidth = 5;

    overlayCtx.moveTo(quad.p00, quad.p01);
    overlayCtx.lineTo(quad.p10, quad.p11);
    overlayCtx.lineTo(quad.p20, quad.p21);
    overlayCtx.lineTo(quad.p30, quad.p31);
    overlayCtx.lineTo(quad.p00, quad.p01);

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
    glitterDetector.start();
}
