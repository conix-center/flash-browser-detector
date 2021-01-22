import {GrayScaleMedia} from "./grayscale";
import {GlitterModule} from "./glitter-module";
import {Timer} from "./timer";
import {DeviceIMU} from "./imu";

export class GlitterDetector {
    constructor(code, targetFps, width, height, video) {
        let _this = this;

        this.code = code;
        this.fpsInterval = 1000 / targetFps; // ms

        this.width = width;
        this.height = height;

        this.imageData = null;
        this.printPerformance = false;

        if (video) {
            this.video = video;
        }
        else {
            this.video = document.createElement("video");
            this.video.setAttribute("autoplay", "");
            this.video.setAttribute("muted", "");
            this.video.setAttribute("playsinline", "");
        }

        this.glitterModule = null;
        this.imu = null;

        this.grayScaleMedia = new GrayScaleMedia(this.video, this.width, this.height);
    }

    onInit(source) {
        function startTick() {
            this.prev = Date.now();
            this.timer = new Timer(this.tick.bind(this), this.fpsInterval);
            this.timer.run();
        }

        this.imu = new DeviceIMU();
        this.glitterModule = new GlitterModule(this.code, this.width, this.height, startTick.bind(this));

        const initEvent = new CustomEvent("onGlitterInit", {detail: {source: source}});
        window.dispatchEvent(initEvent);
    }

    resize(width, height) {
        this.width = width;
        this.height = height;
        this.glitterModule.resize(this.width, this.height);
        this.grayScaleMedia.resize(this.width, this.height);
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

    tick() {
        const start = Date.now();
        // console.log(start - this.prev, this.timer.getError());
        this.prev = start;

        this.imageData = this.grayScaleMedia.getPixels();
        this.glitterModule.saveGrayscale(this.imageData);

        const mid = Date.now();

        const tags = this.glitterModule.detect_tags();

        const end = Date.now();

        if (this.printPerformance || end-start > this.fpsInterval) {
            console.log("[performance]", "Get Pixels:", mid-start, "Detect:", end-mid, "Total:", end-start);
        }

        if (tags) {
            const tagEvent = new CustomEvent("onGlitterTagsFound", {detail: {tags: tags}});
            window.dispatchEvent(tagEvent);
        }
    }
}
