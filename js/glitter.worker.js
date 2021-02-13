import {GlitterModule} from './glitter-module';

onmessage = (e) => {
    const msg = e.data;
    switch (msg.type) {
        case 'init': {
            init(msg);
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
var tags = null;

function init(msg) {
    glitterModule = new GlitterModule(
        msg.codes,
        msg.width,
        msg.height,
        msg.options,
        () => {
            postMessage({type: "loaded"});
        }
    );
}

function process() {
    if (glitterModule) {
        glitterModule.saveGrayscale(next);
        tags = glitterModule.detect_tags();
        postMessage({type: "result", tags: tags});
    }
}
