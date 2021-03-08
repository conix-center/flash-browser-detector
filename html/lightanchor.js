const OFF_COLOR = "#111";
const ON_COLOR = "#fff";
const BORDER_COLOR = "#000";

class LightAnchor {
    constructor(code, codeLen, freq, id, x, y) {
        this.code = code;
        this.codeLen = codeLen;

        this.freq = freq;
        this.fpsInterval = 1000 / this.freq; // ms

        this.adjustedCode = this.preprocess(this.code);
        this.adjustedCodeLen = 2 * this.codeLen;

        this.running = false;

        this.id = id;
        this.createTag(this.id, x, y);
    }

    createTag(id, x, y) {
        this.tag = document.createElement("div");
        this.tag.id = id;

        this.tag.style.position = "absolute";
        this.move(x, y);

        this.tag.style.width = "10vmin";
        this.tag.style.height = "10vmin";
        this.tag.style.border = "5vmin";
        this.tag.style.borderStyle = "solid";
        this.tag.style.borderColor = BORDER_COLOR;
        this.tag.style.background = OFF_COLOR;

        document.body.appendChild(this.tag);
    }

    move(x, y) {
        if (typeof x == "string")
            this.tag.style.left = x
        else
            this.tag.style.left = x + "px";

        if (typeof y == "string")
            this.tag.style.top = y
        else
            this.tag.style.top = y + "px";
    }

    blink(callback) {
        this.totalDt = 0; this.iters = 0;
        this.running = true;

        let idx = 0;
        let expected = Date.now() + this.fpsInterval;

        let _this = this;
        setTimeout(tick, this.fpsInterval);
        function tick() {
            if (!_this.running) return;
            const dt = Date.now() - expected;
            if (dt > _this.fpsInterval)  // adjust for errors
                expected += _this.fpsInterval * Math.floor(dt / _this.fpsInterval);

            const startCompute = Date.now();
            _this.totalDt += Math.abs(dt); _this.iters++;

            const bit = Number(!!(_this.adjustedCode & (1 << (_this.adjustedCodeLen-idx-1))));
            if (bit == 1)
                _this.tag.style.background = ON_COLOR;
            else
                _this.tag.style.background = OFF_COLOR;
            idx = (idx + 1) % _this.adjustedCodeLen;

            if (callback)
                callback(_this.totalDt, _this.iters);

            expected += _this.fpsInterval;
            const computationTime = Date.now() - startCompute;
            setTimeout(tick, Math.max(0, _this.fpsInterval - dt - computationTime)); // take into account drift
        }
    }

    stop() {
        this.running = false;
        this.tag.style.background = OFF_COLOR
    }

    preprocess(code) {
        let res = 0;
        for (let i = 0; i < 8; i++) {
            const currBit = !!(code & (1 << (8-i-1)));
            const nextIdx = (i + 1) % 8;
            const nextBit = !!(code & (1 << (8-nextIdx-1)));

            res |= (currBit << (16-2*i-1));

            if (currBit != nextBit) {
                if (currBit == 1)
                    res |= (0 << (16-2*i-2));
                // else if (currBit == 0)
                //     res |= (1 << (16-2*i-2));
            }
            else {
                res |= (currBit << (16-2*i-2));
            }
        }
        return res;
    }
}
