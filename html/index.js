var codes = [0xaf, 123];
var targetFps = 30;

var stats = null;

var glitterSource = new Glitter.GlitterSource();

var overlayCanvas = document.createElement("canvas");
overlayCanvas.id = "overlay";
overlayCanvas.style.position = "absolute";
overlayCanvas.style.top = "0px";
overlayCanvas.style.left = "0px";
overlayCanvas.width = glitterSource.options.width;
overlayCanvas.height = glitterSource.options.height;

var glitterDetector = new Glitter.GlitterDetector(codes, targetFps, glitterSource);
glitterDetector.setOptions({
    // printPerformance: true,
});
glitterDetector.init();

function drawQuad(quad) {
    var overlayCtx = overlayCanvas.getContext("2d");

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
        // overlayCtx.fillText(Glitter.Utils.dec2bin(code), quad.center.x, quad.center.y);
    overlayCtx.stroke();
}

function drawQuads(quads) {
    var overlayCtx = overlayCanvas.getContext("2d");
    overlayCtx.clearRect(0, 0, overlayCanvas.width, overlayCanvas.height);

    for (var i = 0; i < quads.length; i++) {
        drawQuad(quads[i]);
    }
}

window.addEventListener("onGlitterInit", (e) => {
    stats = new Stats();
    stats.showPanel(0);
    document.getElementById("stats").appendChild(stats.domElement);

    document.body.appendChild(e.detail.source);

    document.body.appendChild(overlayCanvas);

    var info = document.getElementById("info");
    info.style.zIndex = "1";
    info.innerText = "Detecting Codes:\n";
    for(var i = 0; i < this.codes.length; i++) {
        var code = this.codes[i];
        info.innerText += `${Glitter.Utils.dec2bin(code)} (${code})\n`;
    }

    resize();
});

window.addEventListener("onGlitterTagsFound", (e) => {
    drawQuads(e.detail.tags);
    stats.update();
});

window.addEventListener("onGlitterCalibrate", (e) => {
    var info = document.getElementById("info");
    info.innerText = "Detecting Codes:\n";
    for(var i = 0; i < this.codes.length; i++) {
        var code = this.codes[i];
        info.innerText += `${Glitter.Utils.dec2bin(code)} (${code})\n`;
    }
    info.innerText += e.detail.decimationFactor;
});

function resize() {
    glitterSource.resize(window.innerWidth, window.innerHeight);
    glitterSource.copyDimensionsTo(overlayCanvas);
}

window.addEventListener("resize", (e) => {
    resize();
});
