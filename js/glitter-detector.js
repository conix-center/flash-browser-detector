import {GrayScaleMedia} from "./grayscale";
import {GlitterModule} from "./glitter-module";
import {Timer} from "./timer";
import {DeviceIMU} from "./imu";
import {Utils} from './utils/utils';

export class GlitterDetector {
    constructor(code, targetFps, width, height, video) {
        this.code = code;
        this.targetFps = targetFps; // FPS/Hz
        this.fpsInterval = 1000 / this.targetFps; // ms

        this.origWidth = width;
        this.origHeight = height;

        this.width = width;
        this.height = height;

        this.imageData = null;
        this.imageDecimate = 1.0;

        this.options = {
            printPerformance: false,
            maxImageDecimationFactor: 10,
            imageDecimationDelta: 0.3,
            rangeThreshold: 45,
            quadSigma: 1.0,
            refineEdges: 1,
            decodeSharpening: 0.25,
            minWhiteBlackDiff: 20,
        }

        if (video) {
            this.video = video;
        }
        else {
            this.video = document.createElement("video");
            this.video.setAttribute("autoplay", "");
            this.video.setAttribute("muted", "");
            this.video.setAttribute("playsinline", "");
        }

        this.imu = new DeviceIMU();
        this.grayScaleMedia = new GrayScaleMedia(this.video, this.width, this.height);
    }

    setOptions(options) {
        this.options = Object.assign(this.options, options);
    }

    start() {
        this.grayScaleMedia.requestStream()
            .then(source => {
                this.onInit(source);
            })
            .catch(err => {
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

        this.glitterModule = new GlitterModule(this.code, this.width, this.height, this.options, startTick);
        this.imu.start();

        const initEvent = new CustomEvent("onGlitterInit", {detail: {source: source}});
        window.dispatchEvent(initEvent);
    }

    resize(width, height) {
        this.width = width;
        this.height = height;
        this.grayScaleMedia.resize(this.width, this.height);
        this.glitterModule.resize(this.width, this.height);
        this.glitterModule.setQuadDecimate(this.imageDecimate);
    }

    tick() {
        const start = Date.now();
        // console.log(start - this.prev, this.timer.getError());
        this.prev = start;

        this.imageData = this.grayScaleMedia.getPixels();
        this.glitterModule.saveGrayscale(this.imageData);

        const mid = Date.now();

        const tags = this.glitterModule.detect_tags();

        const end = Date.now();

        if (this.options.printPerformance) {
            console.log("[performance]", "Get Pixels:", mid-start, "Detect:", end-mid, "Total:", end-start);
        }

        if (end-start > this.fpsInterval) {
            if (this.imageDecimate < this.options.maxImageDecimationFactor) {
                this.imageDecimate += this.options.imageDecimationDelta;
                this.imageDecimate = Utils.round2(this.imageDecimate);
                this.resize(this.origWidth/this.imageDecimate, this.origHeight/this.imageDecimate)

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
