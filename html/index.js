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
    printPerformance: true,
});
glitterDetector.init();

function drawTag(tag) {
    var overlayCtx = overlayCanvas.getContext("2d");

    overlayCtx.beginPath();
        overlayCtx.lineWidth = 5;
        overlayCtx.strokeStyle = "blue";
        overlayCtx.moveTo(tag.corners[0].x, tag.corners[0].y);
        overlayCtx.lineTo(tag.corners[1].x, tag.corners[1].y);
        overlayCtx.lineTo(tag.corners[2].x, tag.corners[2].y);
        overlayCtx.lineTo(tag.corners[3].x, tag.corners[3].y);
        overlayCtx.lineTo(tag.corners[0].x, tag.corners[0].y);

        overlayCtx.font = "bold 20px Arial";
        overlayCtx.textAlign = "center";
        overlayCtx.fillStyle = "red";
        overlayCtx.fillText(Glitter.Utils.dec2bin(tag.code), tag.center.x, tag.center.y);
    overlayCtx.stroke();
}

function drawTags(tags) {
    var overlayCtx = overlayCanvas.getContext("2d");
    overlayCtx.clearRect(0, 0, overlayCanvas.width, overlayCanvas.height);

    for (var i = 0; i < tags.length; i++) {
        drawTag(tags[i]);
    }
}

function updateInfo() {
    var info = document.getElementById("info");
    info.style.zIndex = "1";
    info.innerText = "Detecting Codes:\n";
    for(var i = 0; i < this.codes.length; i++) {
        var code = this.codes[i];
        info.innerText += `${Glitter.Utils.dec2bin(code)} (${code})\n`;
    }
}

window.addEventListener("onGlitterInit", (e) => {
    stats = new Stats();
    stats.showPanel(0);
    document.getElementById("stats").appendChild(stats.domElement);

    document.body.appendChild(e.detail.source);

    document.body.appendChild(overlayCanvas);

    updateInfo();

    resize();
});

window.addEventListener("onGlitterTagsFound", (e) => {
    drawTags(e.detail.tags);
    stats.update();
});

window.addEventListener("onGlitterCalibrate", (e) => {
    updateInfo();
    info.innerText += e.detail.decimationFactor;
});

function resize() {
    glitterSource.resize(window.innerWidth, window.innerHeight);
    glitterSource.copyDimensionsTo(overlayCanvas);
}

window.addEventListener("resize", (e) => {
    resize();
});
