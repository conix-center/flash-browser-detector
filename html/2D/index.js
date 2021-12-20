var codes = [0b10101010, 0b10011010, 0b10100011];
var targetFps = 30;

var stats = null;

var flashSource = new Flash.FlashSource();
flashSource.setOptions({
    // width: 4032,
    // height: 3024,
    // width: 1920,
    // height: 1080,
    // width: 1280,
    // height: 720,
    width: window.innerWidth,
    height: window.innerHeight,
});

var arElem1 = document.createElement("div");
arElem1.id = "arElem1";
arElem1.className = "arElem";
arElem1.style.backgroundColor = "yellow";

var arElem2 = document.createElement("div");
arElem2.id = "arElem2";
arElem2.className = "arElem";
arElem2.style.backgroundColor = "blue";

var arElem3 = document.createElement("div");
arElem3.id = "arElem3";
arElem3.className = "arElem";
arElem3.style.backgroundColor = "green";

var arElems = [arElem1, arElem2, arElem3];

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

function transformElem(h, elem) {
    // column major order
    let transform = [h[0], h[3], 0, h[6],
                     h[1], h[4], 0, h[7],
                      0  ,  0  , 1,  0  ,
                     h[2], h[5], 0, h[8]];
    transform = "matrix3d("+transform.join(",")+")";
    elem.style["-ms-transform"] = transform;
    elem.style["-webkit-transform"] = transform;
    elem.style["-moz-transform"] = transform;
    elem.style["-o-transform"] = transform;
    elem.style.transform = transform;
    elem.style.display = "block";
}

function drawTag(tag) {
    for (var i = 0; i < codes.length; i++) {
        if (tag.code == codes[i]) {
            transformElem(tag.H, arElems[i]);
        }
    }
}

function drawTags(tags) {
    for (tag of tags) {
        drawTag(tag);
    }
}

window.addEventListener("onFlashInit", (e) => {
    stats = new Stats();
    stats.showPanel(0);
    document.getElementById("stats").appendChild(stats.domElement);

    document.body.appendChild(e.detail.source);
    // document.body.appendChild(flashDetector.preprocessor.canvas);

    for (arElem of arElems) {
        document.body.appendChild(arElem);
    }

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
}

window.addEventListener("resize", (e) => {
    resize();
});
