export class GlitterModule {
    constructor(code, callback) {
        let _this = this;

        this.ready = false;
        this.code = code;

        GlitterWASM().then(function(Module) {
            console.log("GLITTER WASM module loaded.");
            _this.onWasmInit(Module);
            if (callback) callback();
        });
    }

    onWasmInit(Module) {
        this._Module = Module;
        this._init = Module.cwrap("init", "number", ["number"]);
        this._track = this._Module.cwrap("track", "number", ["number", "number", "number"]);
        this.ready = (this._init(this.code) == 0);
    }

    track(im_arr, width, height) {
        let quads = [];
        if (!this.ready) return quads;

        const im_ptr = this._Module._malloc(im_arr.length);
        this._Module.HEAPU8.set(im_arr, im_ptr);

        const ptr = this._track(im_ptr, width, height);
        const ptrF64 = ptr / Float64Array.BYTES_PER_ELEMENT;

        const numQuads = this._Module.getValue(ptr, "double");
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
