if ('function' === typeof importScripts) {
    importScripts("./glitter_wasm.js");
    importScripts("../dist/glitter.min.js");

    self.onmessage = function (e) {
        var msg = e.data;
        switch (msg.type) {
            case "init": {
                load(msg);
                return;
            }
            case "process": {
                next = msg.imagedata;
                process();
                return;
            }
            default: {
                break;
            }
        }
    };

    var next = null;
    var glitterDetector = null;
    var result = null;

    function load(msg) {
        var onLoad = function() {
            postMessage({ type: "loaded" });
        }

        glitterDetector = new Glitter.GlitterDetector(msg.code, msg.width, msg.height, onLoad);
    }

    function process(width, height) {
        result = null;

        if (glitterDetector) {
            result = glitterDetector.track(next, width, height);
        }

        if (result) {
            postMessage({ type: "result", result: result });
        }
        else {
            postMessage({ type: "not found" });
        }

        next = null;
    }

}
