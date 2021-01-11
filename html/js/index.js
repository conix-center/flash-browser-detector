let width = window.innerWidth;
let height = window.innerHeight;

const code = 0xaf;

let stats = null;
let videoSource = null;
let videoCanvas = null;
let overlayCanvas = null;

const targetFps = 30;
const fpsInterval = 1000 / targetFps; // ms

let start_t, prev_t;
let imageData = null;

let glitterDetector = null;

function initStats() {
    stats = new Stats();
    stats.showPanel(0);
    document.getElementById("stats").appendChild(stats.domElement);
}

function clearOverlayCtx(overlayCtx) {
    overlayCtx.clearRect( 0, 0, width, height );
}

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
    clearOverlayCtx(overlayCtx);

    for (var i = 0; i < quads.length; i++) {
        drawQuad(quads[i]);
    }
}

function tick() {

    // const now = Date.now();
    // const dt = now - prev_t;

    // if (dt >= fpsInterval) {
        // prev_t = now - (dt % fpsInterval);

    stats.begin();

    imageData = grayscale.getFrame();
    const videoCanvasCtx = videoCanvas.getContext("2d");
    videoCanvasCtx.drawImage(
        videoSource, 0, 0, width, height
    );
    const quads = glitterDetector.track(imageData, width, height);
    drawQuads(quads);

    stats.end();
    // }

    // requestAnimationFrame(tick);
}

function onInit(source) {
    videoSource = source;
    glitterDetector = new Glitter.GlitterDetector(code, () => {
        start_t = Date.now()
        prev_t = start_t;
        // tick();
        setInterval(tick, fpsInterval)
    });
}

window.onload = () => {
    function setVideoStyle(elem) {
        elem.style.position = "absolute";
        elem.style.top = 0;
    }

    var video = document.createElement("video");
    video.setAttribute("autoplay", "");
    video.setAttribute("muted", "");
    video.setAttribute("playsinline", "");

    videoCanvas = document.createElement("canvas");
    setVideoStyle(videoCanvas);
    videoCanvas.id = "video-canvas";
    videoCanvas.width = width;
    videoCanvas.height = height;
    document.body.appendChild(videoCanvas);

    overlayCanvas = document.createElement("canvas");
    setVideoStyle(overlayCanvas);
    overlayCanvas.id = "overlay";
    overlayCanvas.width = width;
    overlayCanvas.height = height;
    overlayCanvas.style.zIndex = 1;
    document.body.appendChild(overlayCanvas);

    grayscale = new Glitter.GrayScaleMedia(video, width, height);
    grayscale.requestStream()
        .then(source => {
            initStats();
            onInit(source);
        })
        .catch(err => {
            console.warn("ERROR: " + err);
        });
}
