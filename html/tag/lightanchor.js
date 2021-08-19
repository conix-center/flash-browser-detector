const OFF_COLOR = "#111111";
const ON_COLOR = "#ffffff";
const BORDER_COLOR = "#000000";

function dec2bin(dec) {
    return (dec >>> 0).toString(2);
}

class LightAnchor {
    constructor(code, codeLen, freq, id, x, y, border) {
        this.code = code;
        this.codeLen = codeLen;

        this.freq = freq;
        this.fpsInterval = 1000 / this.freq; // ms

        this.adjustedCode = this.preprocess(this.code);
        this.adjustedCodeLen = 2 * this.codeLen;

        this.running = false;

        this.id = id;
        this.createTag(this.id, x, y, border);
    }

    createTag(id, x, y, border) {
        this.wrapper = document.createElement("div");
        this.wrapper.id = `${id} tag`;
        this.wrapper.style.width = "35vmin";
        this.wrapper.style.height = "35vmin";
        this.wrapper.style.position = "absolute";
        this.wrapper.style.justifyContent = "center";
        this.wrapper.style.justifyText = "center";
        this.wrapper.style.backgroundColor = "white";
        this.wrapper.style.transform = "translate(-50%,-50%)";
        this.move(x, y);
        document.body.appendChild(this.wrapper);

        this.tagWrapper = document.createElement("div");
        this.tagWrapper.id = `${id} wrapper`;
        this.tagWrapper.style.position = "absolute";
        this.tagWrapper.style.justifyContent = "center";
        this.tagWrapper.style.justifyText = "center";
        this.tagWrapper.style.left = "50%";
        this.tagWrapper.style.top = "50%";
        this.tagWrapper.style.transform = "translate(-50%,-50%)";
        this.wrapper.appendChild(this.tagWrapper);

        this.tag = document.createElement("div");
        this.tag.id = id;
        this.tag.style.width = "10vmin";
        this.tag.style.height = "10vmin";
        this.tag.style.border = border ? border : "7vmin";
        this.tag.style.borderStyle = "solid";
        this.tag.style.borderColor = BORDER_COLOR;
        this.tag.style.backgroundColor = OFF_COLOR;
        this.tagWrapper.appendChild(this.tag);

        this.label = document.createElement("p");
        this.label.id = `${id} label`;
        this.label.innerText = `${dec2bin(this.code)} (${this.code})`;
        this.label.style.fontSize = "x-small";
        this.label.style.position = "absolute";
        this.label.style.top = "100%";
        this.wrapper.appendChild(this.label);
    }

    copyDimensionsTo(elem) {
        elem.style.width = this.wrapper.style.width;
        elem.style.height = this.wrapper.style.height;
        elem.style.position = this.wrapper.style.position;
    }

    setOuterWidth(width) {
        if (typeof width == "string")
            width = width;
        else
            width = width + "px";
        this.wrapper.style.width = width;
        this.wrapper.style.height = width;
    }

    setInnerWidth(width) {
        if (typeof width == "string")
            width = width;
        else
            width = width + "px";
        this.tag.style.width = width;
        this.tag.style.height = width;
    }

    setBorderWidth(width) {
        if (typeof width == "string")
            width = width;
        else
            width = width + "px";
        this.tag.style.border = width;
    }

    move(x, y) {
        if (typeof x == "string")
            this.wrapper.style.left = x;
        else
            this.wrapper.style.left = x + "px";

        if (typeof y == "string")
            this.wrapper.style.top = y;
        else
            this.wrapper.style.top = y + "px";
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
            // const nextBit = !!(code & (1 << (8-nextIdx-1)));

            res |= (currBit << (16-2*i-1));

            // if (currBit != nextBit) {
            //     if (currBit == 1)
            //         res |= (0 << (16-2*i-2));
            //     // else if (currBit == 0)
            //     //     res |= (1 << (16-2*i-2));
            // }
            // else {
            //     res |= (currBit << (16-2*i-2));
            // }
        }
        return res;
    }
}
