import {Utils} from "./utils/utils";
import {Timer} from "./utils/timer";
import Worker from "./flash.worker";
// import {DeviceIMU} from "./imu";

export class FlashDetector {
    constructor(codes, targetFps, source, options) {
        this.codes = codes;
        this.targetFps = targetFps; // FPS
        this.fpsInterval = 1000 / this.targetFps; // ms

        this.source = source;
        this.sourceWidth = this.source.options.width;
        this.sourceHeight = this.source.options.height;

        this.imageDecimate = 1.0;

        this.options = {
            decimateImage: false,
            maxImageDecimationFactor: 3,
            imageDecimationDelta: 0.2,
            rangeThreshold: 20,
            minWhiteBlackDiff: 100,
            ttlFrames: 8,
            thresDistShape: 50.0,
            thresDistShapeTTL: 20.0,
            thresDistCenter: 25.0,
        }
        this.setOptions(options);

        // this.imu = new DeviceIMU();
        this.worker = new Worker();
    }

    init() {
        this.source.init()
            .then((source) => {
                this.onInit(source);
            })
            .catch((err) => {
                console.warn("ERROR: " + err);
            });
    }

    createTimer(callback) {
        return new Timer(callback, this.fpsInterval);
    }

    onInit(source) {
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

        this.source.preprocessor.resize(width, height);
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
        }
    }

    addCode(code) {
        this.worker.postMessage({
            type: "add code",
            code: code
        });
    }

    detectTags(imageData) {
        this.worker.postMessage({
            type: "process",
            imagedata: imageData
        });
    }
}
