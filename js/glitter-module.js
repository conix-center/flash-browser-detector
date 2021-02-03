export class GlitterModule {
    constructor(codes, width, height, options, callback) {
        this.width = width;
        this.height = height;

        this.ready = false;
        this.codes = codes;

        this.options = options;

        let _this = this;
        GlitterWASM().then(function(Module) {
            console.log("GLITTER WASM module loaded.");
            _this.onWasmInit(Module);
            if (callback) callback();
        });
    }

    onWasmInit(Module) {
        this._Module = Module;

        this._init = this._Module.cwrap("init", "number", ["number"]);
        this._add_code = this._Module.cwrap("add_code", "number", ["number"]);

        this._set_detector_options = this._Module.cwrap("set_detector_options", "number", ["number", "number", "number", "number"]);
        this._set_quad_decimate = this._Module.cwrap("set_quad_decimate", "number", ["number"]);

        this._save_grayscale = this._Module.cwrap("save_grayscale", "number", ["number", "number", "number", "number"]);

        this._detect_tags = this._Module.cwrap("detect_tags", "number", ["number", "number", "number"]);

        this.ready = (this._init() == 0);
        this.setDetectorOptions(this.options); // set default options

        for (var i = 0; i < this.codes.length; i++) {
            this._add_code(this.codes[i]);
        }

        this.imagePtr = this._Module._malloc(this.width * this.height * 4);
        this.grayPtr = this._Module._malloc(this.width * this.height);

        this.tags = [];

        let _this = this;
        window.addEventListener("onGlitterTagFound", (e) => {
            _this.tags.push(e.detail.tag);
        });
    }

    resize(width, height) {
        this.width = width;
        this.height = height;

        this._Module._free(this.imagePtr);
        this._Module._free(this.grayPtr);

        this.imagePtr = this._Module._malloc(this.width * this.height * 4);
        this.grayPtr = this._Module._malloc(this.width * this.height);
    }

    addCode(code) {
        if (0x00 < code < 0xff) {
            this._add_code(code);
        }
        return this.codes.push(code);
    }

    setDetectorOptions(options) {
        this._set_detector_options(options.rangeThreshold, options.quadSigma, options.refineEdges, options.decodeSharpening, options.minWhiteBlackDiff);
    }

    setQuadDecimate(factor) {
        return this._set_quad_decimate(factor);
    }

    saveGrayscale(pixels) {
        this._Module.HEAPU8.set(pixels, this.imagePtr);
        return this._save_grayscale(this.imagePtr, this.grayPtr, this.width, this.height);
    }

    detect_tags() {
        this.tags = []; // reset found tags
        if (!this.ready) return this.tags;

        this._detect_tags(this.grayPtr, this.width, this.height); // detect new tags

        return this.tags;
    }
}
