export class GlitterModule {
    constructor(code, width, height, options, callback) {
        this.width = width;
        this.height = height;

        this.ready = false;
        this.code = code;

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

        this._set_detector_options = this._Module.cwrap("set_detector_options", "number", ["number", "number", "number", "number"]);
        this._set_quad_decimate = this._Module.cwrap("set_quad_decimate", "number", ["number"]);

        this._save_grayscale = this._Module.cwrap("save_grayscale", "number", ["number", "number", "number", "number"]);

        this._detect_tags = this._Module.cwrap("detect_tags", "number", ["number", "number", "number"]);

        this.ready = (this._init(this.code) == 0);
        this.setDetectorOptions(this.options); // set default options

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
        let tags = [];
        if (!this.ready) return tags;

        const ptr = this._detect_tags(this.grayPtr, this.width, this.height);
        const ptrF64 = ptr / Float64Array.BYTES_PER_ELEMENT;

        const numTags = this._Module.getValue(ptr, "double");

        for (var i = 0; i < numTags; i++) {
            const p0x = this._Module.HEAPF64[ptrF64+10*i+1+0];
            const p0y = this._Module.HEAPF64[ptrF64+10*i+1+1];
            const p1x = this._Module.HEAPF64[ptrF64+10*i+1+2];
            const p1y = this._Module.HEAPF64[ptrF64+10*i+1+3];
            const p2x = this._Module.HEAPF64[ptrF64+10*i+1+4];
            const p2y = this._Module.HEAPF64[ptrF64+10*i+1+5];
            const p3x = this._Module.HEAPF64[ptrF64+10*i+1+6];
            const p3y = this._Module.HEAPF64[ptrF64+10*i+1+7];
            const cx  = this._Module.HEAPF64[ptrF64+10*i+1+8];
            const cy  = this._Module.HEAPF64[ptrF64+10*i+1+9];
            var tag = {
                corners: [
                        {x: p0x, y: p0y},
                        {x: p1x, y: p1y},
                        {x: p2x, y: p2y},
                        {x: p3x, y: p3y}
                    ],
                center: {x: cx, y: cy}
            };
            tags.push(tag);
        }

        this._Module._free(ptr);

        return tags;
    }
}
