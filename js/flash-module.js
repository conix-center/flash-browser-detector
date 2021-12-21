import FlashWASM from "../build/flash_wasm";

export class FlashModule {
    constructor(codes, width, height, options, callback) {
        this.width = width;
        this.height = height;

        this.scope;
        if ('function' === typeof importScripts)
            this.scope = self;
        else
            this.scope = window;

        this.options = options;

        this.ready = false;
        this.codes = codes;

        this.scope = ('function' === typeof importScripts) ? self : window;

        let _this = this;
        FlashWASM().then(function(Module) {
            console.info("FLASH WASM module loaded.");
            _this.onWasmInit(Module);
            if (callback) callback();
        });
    }

    onWasmInit(Module) {
        this._Module = Module;

        this.ready = (this._Module.init() == 0);
        this.setDetectorOptions(this.options); // set default options

        for (var i = 0; i < this.codes.length; i++) {
            this._Module.add_code(this.codes[i]);
        }

        this.imagePtr = this._Module._malloc(this.width * this.height * 4);
        this.grayPtr = this._Module._malloc(this.width * this.height);
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
            this._Module.add_code(code);
            return this.codes.push(code);
        }
        return -1;
    }

    setDetectorOptions(options) {
        this._Module.set_detector_options(
            options.rangeThreshold,
            options.minWhiteBlackDiff,
            options.ttlFrames,
            options.thresDistShape,
            options.thresDistShapeTTL,
            options.thresDistCenter
        );
    }

    setQuadDecimate(factor) {
        return this._Module.set_quad_decimate(factor);
    }

    saveGrayscale(pixels) {
        this._Module.HEAPU8.set(pixels, this.imagePtr);
        return this._Module.save_grayscale(this.imagePtr, this.grayPtr, this.width, this.height);
    }

    detectTags() {
        if (!this.ready) {
            this.tags = this.scope.tags;
            return this.scope.tags;
        }

        this._Module.detect_tags(this.grayPtr, this.width, this.height); // detect new tags
        this.tags = this.scope.tags;
        return this.scope.tags;
    }
}
