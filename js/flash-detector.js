
import {Timer} from "./timer";
import {Utils} from "./utils/utils";
// import {DeviceIMU} from "./imu";
import {Preprocessor} from "./preprocessor";
import Worker from "./flash.worker";

var BAD_FRAMES_BEFORE_DECIMATE = 20;

export class FlashDetector {
    constructor(codes, targetFps, source, options) {
        this.codes = codes;
        this.targetFps = targetFps; // FPS
        this.fpsInterval = 1000 / this.targetFps; // ms

        this.source = source;
        this.sourceWidth = this.source.options.width;
        this.sourceHeight = this.source.options.height;

        this.imageDecimate = 1.0;

        this.numBadFrames = 0;

        this.options = {
            printPerformance: false,
            decimateImage: false,
            maxImageDecimationFactor: 3,
            imageDecimationDelta: 0.2,
            rangeThreshold: 20,
            quadSigma: 1.0,
            minWhiteBlackDiff: 100,
            ttlFrames: 8,
            thresDistShape: 50.0,
            thresDistShapeTTL: 20.0,
            thresDistCenter: 25.0,
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
            targetFps: this.targetFps,
            options: this.options
        });

        this.worker.onmessage = (e) => {
            const msg = e.data
            switch (msg.type) {
                case "loaded": {
                    // this.imu.init();
                    startTick();
                    const initEvent = new CustomEvent(
                        "onFlashInit",
                        {detail: {source: source}}
                    );
                    window.dispatchEvent(initEvent);
                    break;
                }
                case "result": {
                    const tagEvent = new CustomEvent(
                        "onFlashTagsFound",
                        {detail: {tags: msg.tags}}
                    );
                    window.dispatchEvent(tagEvent);
                    break;
                }
                case "resize": {
                    this.decimate();
                    break;
                }
            }
        }
    }

    decimate() {
        this.imageDecimate += this.options.imageDecimationDelta;
        this.imageDecimate = Utils.round3(this.imageDecimate);
        var width = this.sourceWidth / this.imageDecimate;
        var height = this.sourceHeight / this.imageDecimate;

        this.preprocessor.resize(width, height);
        this.worker.postMessage({
            type: "resize",
            width: width,
            height: height,
            decimate: this.imageDecimate,
        });

        const calibrateEvent = new CustomEvent(
                "onFlashCalibrate",
                {detail: {decimationFactor: this.imageDecimate}}
            );
        window.dispatchEvent(calibrateEvent);
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

        this.preprocessor.getPixels().then((imageData) => {
            this.worker.postMessage({
                type: "process",
                imagedata: imageData
            });
        });

        const end = Date.now();

        if (this.options.printPerformance) {
            console.log("[performance]", "Get Pixels:", end-start);
        }

        const tickEvent = new CustomEvent(
            "onFlashTick",
            {detail: {}}
        );
        window.dispatchEvent(tickEvent);

        if (this.options.decimateImage) {
            if (end-start > this.fpsInterval) {
                this.numBadFrames++;
                if (this.numBadFrames > BAD_FRAMES_BEFORE_DECIMATE &&
                    this.imageDecimate < this.options.maxImageDecimationFactor) {
                    this.numBadFrames = 0;
                    this.decimate();
                }
            }
            else {
                this.numBadFrames = 0;
            }
        }
    }
}
