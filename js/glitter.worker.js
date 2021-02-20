import {GlitterModule} from './glitter-module';

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
var glitterModule = null;
var next = null;
var numBadFrames = 0;

function init(msg) {
    function onLoaded() {
        postMessage({type: "loaded"});
    }

    glitterModule = new GlitterModule(
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
    if (glitterModule) {
        glitterModule.addCode(code);
    }
}

function resize(width, height, decimate) {
    if (glitterModule) {
        glitterModule.resize(width, height);
        glitterModule.setQuadDecimate(decimate);
    }
}

function process() {
    if (glitterModule) {
        const start = Date.now();

        glitterModule.saveGrayscale(next);
        const tags = glitterModule.detectTags();
        postMessage({type: "result", tags: tags});

        const end = Date.now();

        if (glitterModule.options.printPerformance) {
            console.log("[performance]", "Detect:", end-start);
        }

        if (glitterModule.options.decimateImage) {
            if (end-start > 2*fpsInterval) {
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
