let width = Math.min(window.innerWidth, window.innerHeight);

const code = 0xaf;

const targetFps = 30;
const fpsInterval = 1000 / targetFps; // ms

let start_t, prev_t;
let frames = 0;

function toggleTracking() {
    window.glitterEnabled = !window.glitterEnabled;
}
window.addEventListener("touchstart", toggleTracking);
window.addEventListener("mousedown", toggleTracking);

function initStats() {
    window.stats = new Stats();
    window.stats.showPanel(0);
    document.getElementById("stats").appendChild(stats.domElement);
}

function setVideoStyle(elem) {
    elem.style.position = "absolute";
    elem.style.top = 0;
    elem.style.left = 0;
}

function setupVideo(displayVid, displayOverlay, setupCallback) {
    window.videoElem = document.createElement("video");
    window.videoElem.setAttribute("autoplay", "");
    window.videoElem.setAttribute("muted", "");
    window.videoElem.setAttribute("playsinline", "");

    navigator.mediaDevices.getUserMedia({
        video: { facingMode: "environment" },
        audio: false
    })
    .then(stream => {
        const videoSettings = stream.getVideoTracks()[0].getSettings();
        window.videoElem.srcObject = stream;
        window.videoElem.play();
    })
    .catch(function(err) {
        console.log("ERROR: " + err);
    });

    window.videoCanv = document.createElement("canvas");
    setVideoStyle(window.videoCanv);
    window.videoCanv.style.zIndex = -1;
    if (displayVid) {
        document.body.appendChild(window.videoCanv);
    }

    if (displayOverlay) {
        window.overlayCanv = document.createElement("canvas");
        setVideoStyle(window.overlayCanv);
        window.overlayCanv.style.zIndex = 0;
        document.body.appendChild(window.overlayCanv);
    }

    window.videoElem.addEventListener("canplay", function(e) {
        window.width = width;
        window.height = window.videoElem.videoHeight / (window.videoElem.videoWidth / window.width);

        window.videoElem.setAttribute("width", window.width);
        window.videoElem.setAttribute("height", window.height);

        window.videoCanv.width = window.width;
        window.videoCanv.height = window.height;

        if (displayOverlay) {
            window.overlayCanv.width = window.width;
            window.overlayCanv.height = window.height;
        }

        if (setupCallback != null) {
            setupCallback();
        }
    }, false);
}

function getFrameGrayscale() {
    const videoCanvCtx = window.videoCanv.getContext("2d");
    videoCanvCtx.drawImage(
        window.videoElem,
        0, 0,
        window.width,
        window.height
    );

    let imageData = videoCanvCtx.getImageData(0, 0, window.width, window.height);
    let imageDataPixels = imageData.data;
    let grayscalePixels = new Uint8Array(videoCanvCtx.canvas.width * videoCanvCtx.canvas.height);

    for (var i = 0, j = 0; i < imageDataPixels.length; i += 4, j++) {
        let r = imageDataPixels[i];
        let g = imageDataPixels[i+1];
        let b = imageDataPixels[i+2];
        let grayscale = (0.30 * r) + (0.59 * g) + (0.11 * b);
        grayscalePixels[j] = grayscale;

        imageDataPixels[i] = grayscale;
        imageDataPixels[i+1] = grayscale;
        imageDataPixels[i+2] = grayscale;
    }

    videoCanvCtx.putImageData(imageData, 0, 0);

    return grayscalePixels;
}

function clearOverlayCtx(overlayCtx) {
    if (!window.overlayCanv) return;
    overlayCtx.clearRect(
        0, 0,
        window.width,
        window.height
    );
}

function drawQuad(quad) {
    const overlayCtx = window.overlayCanv.getContext("2d");

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
    if (!window.overlayCanv) return;
    const overlayCtx = window.overlayCanv.getContext("2d");
    clearOverlayCtx(overlayCtx);

    for (var i = 0; i < quads.length; i++) {
        drawQuad(quads[i]);
    }
}

function processVideo() {
    requestAnimationFrame(processVideo);

    const now = Date.now();
    const dt = now - prev_t;

    if (dt >= fpsInterval) {
        prev_t = now - (dt % fpsInterval);

        window.stats.begin();

        const frame = getFrameGrayscale();
        if (window.glitterEnabled) {
            let quads = window.glitterDetector.track(frame, window.width, window.height);
            drawQuads(quads);
            // console.log(quads.length)
        }
        // console.log(dt)

        window.stats.end();
    }
}

window.onload = function() {
    window.glitterDetector = new GLITTER_Detector(code, () => {
        initStats();
        setupVideo(true, true, () => {
            start_t = Date.now()
            prev_t = start_t;
            requestAnimationFrame(processVideo);
        });
    });
}
