
import {Timer} from "./timer";
import {Utils} from "./utils/utils";
// import {DeviceIMU} from "./imu";
import {Preprocessor} from "./preprocessor";
import Worker from "./glitter.worker";

export class GlitterDetector {
    constructor(codes, targetFps, source, options) {
        this.codes = codes;
        this.targetFps = targetFps; // FPS
        this.fpsInterval = 1000 / this.targetFps; // ms

        this.source = source;
        this.sourceWidth = this.source.options.width;
        this.sourceHeight = this.source.options.height;

        this.imageData = null;
        this.imageDecimate = 1.0;

        this.numBadFrames = 0;

        this.options = {
            printPerformance: false,
            decimateImage: true,
            maxImageDecimationFactor: 3,
            imageDecimationDelta: 0.2,
            rangeThreshold: 85,
            amplitudeThreshold: 10,
            quadSigma: 1.0,
            refineEdges: true,
            minWhiteBlackDiff: 50,
        }
        this.setOptions(options);

        // this.imu = new DeviceIMU();
        this.preprocessor = new Preprocessor(this.sourceWidth, this.sourceHeight);
        this.preprocessor.setKernelSigma(this.options.quadSigma);
        this.worker = new Worker();
    }

    init() {
        this.source.init()
            .then((source) => {
                this.preprocessor.attachElem(source);
                this.onInit(source);
            })
            .catch((err) => {
                console.warn("ERROR: " + err);
            });
    }

    onInit(source) {
        let _this = this;
        function startTick() {
            _this.prev = Date.now();
            _this.timer = new Timer(_this.tick.bind(_this), _this.fpsInterval);
            _this.timer.run();
        }

        this.worker.postMessage({
            type: "init",
            codes: this.codes,
            width: this.sourceWidth,
            height: this.sourceHeight,
            options: this.options
        });

        this.worker.onmessage = (e) => {
            const msg = e.data
            switch (msg.type) {
                case "loaded": {
                    // this.imu.init();
                    startTick();
                    const initEvent = new CustomEvent("onGlitterInit", {detail: {source: source}});
                    window.dispatchEvent(initEvent);
                    break;
                }
                case "result": {
                    const tagEvent = new CustomEvent("onGlitterTagsFound", {detail: {tags: msg.tags}});
                    window.dispatchEvent(tagEvent);
                    break;
                }
            }
        }
    }

    decimate(width, height) {
        this.preprocessor.resize(width, height);
        // this.glitterModule.resize(width, height);
        // this.glitterModule.setQuadDecimate(this.imageDecimate);
    }

    setOptions(options) {
        if (options) {
            this.options = Object.assign(this.options, options);
            this.preprocessor.setKernelSigma(this.options.quadSigma);
        }
    }

    addCode(code) {
        this.worker.postMessage({
            type: "add code",
            code: code
        });
    }

    tick() {
        const start = Date.now();
        // console.log(start - this.prev, this.timer.getError());
        this.prev = start;

        this.imageData = this.preprocessor.getPixels();
        this.worker.postMessage({
            type: "process",
            imagedata: this.imageData
        });

        const end = Date.now();

        if (this.options.printPerformance) {
            console.log("[performance]", "Get Pixels:", end-start);
        }

        if (this.options.decimateImage && end-start > this.fpsInterval) {
            this.numBadFrames++;
            if (this.numBadFrames > this.targetFps/2 &&
                this.imageDecimate < this.options.maxImageDecimationFactor) {
                this.numBadFrames = 0;

                this.imageDecimate += this.options.imageDecimationDelta;
                this.imageDecimate = Utils.round3(this.imageDecimate);
                this.decimate(
                        this.sourceWidth/this.imageDecimate,
                        this.sourceHeight/this.imageDecimate
                    );

                const calibrateEvent = new CustomEvent(
                        "onGlitterCalibrate",
                        {detail: {decimationFactor: this.imageDecimate}}
                    );
                window.dispatchEvent(calibrateEvent);
            }
        }
    }
}
