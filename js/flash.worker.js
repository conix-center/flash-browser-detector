import {FlashModule} from './flash-module';

onmessage = (e) => {
    const msg = e.data;
    switch (msg.type) {
        case 'init': {
            init(msg);
            return;
        }
        case 'add code': {
            addCode(msg.code);
            return;
        }
        case 'resize': {
            resize(msg.width, msg.height, msg.decimate);
            return;
        }
        case 'process': {
            next = msg.imagedata;
            process();
            return;
        }
    }
}

var BAD_FRAMES_BEFORE_DECIMATE = 20;

var targetFps = null, fpsInterval = null;
var flashModule = null;
var next = null;
var numBadFrames = 0;

function init(msg) {
    function onLoaded() {
        postMessage({type: "loaded"});
    }

    flashModule = new FlashModule(
        msg.codes,
        msg.width,
        msg.height,
        msg.options,
        onLoaded
    );
    targetFps = msg.targetFps;
    fpsInterval = 1000 / targetFps;
}

function addCode(code) {
    if (flashModule) {
        flashModule.addCode(code);
    }
}

function resize(width, height, decimate) {
    if (flashModule) {
        flashModule.resize(width, height);
        flashModule.setQuadDecimate(decimate);
    }
}

function process() {
    if (flashModule) {
        const start = Date.now();

        flashModule.saveGrayscale(next);
        const tags = flashModule.detectTags();
        postMessage({type: "result", tags: tags});

        const end = Date.now();

        if (flashModule.options.printPerformance) {
            console.log("[performance]", "Detect:", end-start);
        }

        if (flashModule.options.decimateImage) {
            if (end-start > fpsInterval) {
                numBadFrames++;
                if (numBadFrames > BAD_FRAMES_BEFORE_DECIMATE) {
                    numBadFrames = 0;
                    postMessage({type: "resize"});
                }
            }
            else {
                numBadFrames = 0;
            }
        }
    }
}
