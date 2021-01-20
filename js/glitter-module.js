export class GlitterModule {
    constructor(code, width, height, callback) {
        let _this = this;

        this.width = width;
        this.height = height;

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

        this._init = this._Module.cwrap("init", "number", ["number"]);
        this._detect_tags = this._Module.cwrap("detect_tags", "number", ["number", "number", "number"]);

        this.ready = (this._init(this.code) == 0);

        this.imPtr = this._Module._malloc(this.width*this.height);
    }

    detect_tags(pixels) {
        let quads = [];
        if (!this.ready) return quads;

        this._Module.HEAPU8.set(pixels, this.imPtr);

        const ptr = this._detect_tags(this.imPtr, this.width, this.height);
        const ptrF64 = ptr / Float64Array.BYTES_PER_ELEMENT;

        const numQuads = this._Module.getValue(ptr, "double");

        for (var i = 0; i < numQuads; i++) {
            const p00 = this._Module.HEAPF64[ptrF64+10*i+1+0];
            const p01 = this._Module.HEAPF64[ptrF64+10*i+1+1];
            const p10 = this._Module.HEAPF64[ptrF64+10*i+1+2];
            const p11 = this._Module.HEAPF64[ptrF64+10*i+1+3];
            const p20 = this._Module.HEAPF64[ptrF64+10*i+1+4];
            const p21 = this._Module.HEAPF64[ptrF64+10*i+1+5];
            const p30 = this._Module.HEAPF64[ptrF64+10*i+1+6];
            const p31 = this._Module.HEAPF64[ptrF64+10*i+1+7];
            const c0  = this._Module.HEAPF64[ptrF64+10*i+1+8];
            const c1  = this._Module.HEAPF64[ptrF64+10*i+1+9];
            var q = {
                corners: [{x:p00,y:p01}, {x:p10,y:p11}, {x:p20,y:p21}, {x:p30,y:p31}],
                center: {x:c0, y:c1}
            };
            quads.push(q);
        }

        this._Module._free(ptr);

        return quads;
    }
}
