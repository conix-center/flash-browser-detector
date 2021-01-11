let width = window.innerWidth;
let height = window.innerHeight;

const code = 0xaf;

let stats = null;
let videoSource = null;
let videoCanvas = null;
let overlayCanvas = null;

const targetFps = 20;
const fpsInterval = 1000 / targetFps; // ms

let imageData = null;

let worker = null;

function initStats() {
    stats = new Stats();
    stats.showPanel(0);
    document.getElementById("stats").appendChild(stats.domElement);
}

function clearOverlay(overlayCtx) {
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
    clearOverlay(overlayCtx);

    for (var i = 0; i < quads.length; i++) {
        drawQuad(quads[i]);
    }
}

function tick() {
    stats.begin();

    imageData = grayscale.getFrame();
    const videoCanvasCtx = videoCanvas.getContext("2d");
    videoCanvasCtx.drawImage(
        videoSource, 0, 0, width, height
    );

    stats.end();

    requestAnimationFrame(tick);
}

function onInit(source) {
    videoSource = source;

    worker = new Worker("../js/glitter.worker.js");
    worker.postMessage({ type: 'init', code: code, width: width, height: height });

    worker.onmessage = function (e) {
        var msg = e.data;
        switch (msg.type) {
            case "loaded": {
                setInterval(process, fpsInterval);
                break;
            }
            case "result": {
                const result = msg.result;
                drawQuads(result);
                break;
            }
            case "not found": {
                clearOverlay();
            }
            default: {
                break;
            }
        }
    }

    tick();
}

function process() {
    if (imageData) {
        worker.postMessage({ type: 'process', imagedata: imageData });
    }
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
