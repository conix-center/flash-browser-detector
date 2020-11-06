class GLITTER_Detector {
    constructor(callback) {
        let _this = this;
        this.ready = false;
        this.shouldTrack = false;
        this.validPoints = false;
        GlitterWASM().then(function (Module) {
            console.log("GLITTER WASM module loaded.");
            _this.onWasmInit(Module);
            if (callback) callback();
        });
    }

    onWasmInit(Module) {
        this._Module = Module;
    }

    init() {
        let res = this._Module.ccall( "init", "number", null, null );
        return res == 0;
    }

    track(im_arr, width, height) {
        const im_ptr = this._Module._malloc(im_arr.length);
        this._Module.HEAPU8.set(im_arr, im_ptr);

        const ptr = this._Module.ccall(
            "track",
            "number",
            ["number", "number", "number"],
            [im_ptr, width, height]
        );
        const ptrF64 = ptr / Float64Array.BYTES_PER_ELEMENT;

        let quads = [];

        const numQuads = this._Module.HEAPF64[ptrF64];
        // console.log("numQuads = ", numQuads);

        for (var i = 0; i < numQuads; i++) {
            var q = {
                p00 : this._Module.HEAPF64[ptrF64+10*i+1+0],
                p01 : this._Module.HEAPF64[ptrF64+10*i+1+1],
                p10 : this._Module.HEAPF64[ptrF64+10*i+1+2],
                p11 : this._Module.HEAPF64[ptrF64+10*i+1+3],
                p20 : this._Module.HEAPF64[ptrF64+10*i+1+4],
                p21 : this._Module.HEAPF64[ptrF64+10*i+1+5],
                p30 : this._Module.HEAPF64[ptrF64+10*i+1+6],
                p31 : this._Module.HEAPF64[ptrF64+10*i+1+7],
                c0  : this._Module.HEAPF64[ptrF64+10*i+1+8],
                c1  : this._Module.HEAPF64[ptrF64+10*i+1+9],
            };
            quads.push(q);
        }

        this._Module._free(ptr);
        this._Module._free(im_ptr);

        return quads;
    }
}
