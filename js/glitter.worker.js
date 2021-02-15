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
        case 'process': {
            next = msg.imagedata;
            process();
            return;
        }
    }
}

var glitterModule = null;
var next = null;

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
}

function addCode(code) {
    if (glitterModule) {
        glitterModule.addCode(code);
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
    }
}
