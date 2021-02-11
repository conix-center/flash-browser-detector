
import {Timer} from "./timer";
import {Utils} from "./utils/utils";
import {DeviceIMU} from "./imu";
import {Preprocessor} from "./preprocessor";
import {GlitterModule} from "./glitter-module";

export class GlitterDetector {
    constructor(codes, targetFps, source, options) {
        this.codes = codes;
        this.targetFps = targetFps; // FPS/Hz
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
            rangeThreshold: 45,
            quadSigma: 1.0,
            refineEdges: 1,
            minWhiteBlackDiff: 50,
        }
        this.setOptions(options);

        this.imu = new DeviceIMU();
        this.preprocessor = new Preprocessor(this.sourceWidth, this.sourceHeight);
        this.preprocessor.setKernelSigma(this.options.quadSigma);
    }

    setOptions(options) {
        if (options) {
            this.options = Object.assign(this.options, options);
            this.preprocessor.setKernelSigma(this.options.quadSigma);
        }
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

        this.glitterModule = new GlitterModule(this.codes, this.sourceWidth, this.sourceHeight, this.options, startTick);
        this.imu.init();

        const initEvent = new CustomEvent("onGlitterInit", {detail: {source: source}});
        window.dispatchEvent(initEvent);
    }

    decimate(width, height) {
        this.preprocessor.resize(width, height);
        this.glitterModule.resize(width, height);
        this.glitterModule.setQuadDecimate(this.imageDecimate);
    }

    addCode(code) {
        return this.glitterModule.addCode(code);
    }

    tick() {
        const start = Date.now();
        // console.log(start - this.prev, this.timer.getError());
        this.prev = start;

        this.imageData = this.preprocessor.getPixels();
        this.glitterModule.saveGrayscale(this.imageData);

        const mid = Date.now();

        const tags = this.glitterModule.detect_tags();

        const end = Date.now();

        if (this.options.printPerformance) {
            console.log("[performance]", "Get Pixels:", mid-start, "Detect:", end-mid, "Total:", end-start);
        }

        if (this.options.decimateImage && end-start > this.fpsInterval) {
            this.numBadFrames++;
            if (this.numBadFrames > this.targetFps/2 &&
                this.imageDecimate < this.options.maxImageDecimationFactor) {
                this.imageDecimate += this.options.imageDecimationDelta;
                this.imageDecimate = Utils.round3(this.imageDecimate);
                this.decimate(this.sourceWidth/this.imageDecimate, this.sourceHeight/this.imageDecimate);
                this.numBadFrames = 0;

                const calibrateEvent = new CustomEvent("onGlitterCalibrate", {detail: {decimationFactor: this.imageDecimate}});
                window.dispatchEvent(calibrateEvent);
            }
        }

        if (tags) {
            const tagEvent = new CustomEvent("onGlitterTagsFound", {detail: {tags: tags}});
            window.dispatchEvent(tagEvent);
        }
    }
}
