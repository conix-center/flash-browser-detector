function dec2bin(dec){
    return (dec >>> 0).toString(2);
}

export class GlitterDetector {
    constructor(code, width, height, callback) {
        this.ready = false;

        this._width = width;
        this._height = height;

        this.code = code;

        let _this = this;
        GlitterWASM().then(function (Module) {
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

        this.imPtr = this._Module._malloc(this._width * this._height);
    }

    track(imArr) {
        let quads = [];
        if (!this.ready) return quads;

        this._Module.HEAPU8.set(imArr, this.imPtr);

        const ptr = this._track(this.imPtr, this._width, this._height);
        const ptrF64 = ptr / Float64Array.BYTES_PER_ELEMENT;

        const numQuads = this._Module.getValue(ptr, "double");

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

        return quads;
    }
}
