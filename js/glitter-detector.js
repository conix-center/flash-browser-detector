import {GrayScaleMedia} from "./grayscale"
import {GlitterModule} from "./glitter-module"

export class GlitterDetector {
    constructor(code, targetFps, width, height, video) {
        this.code = code;
        this.fpsInterval = 1000 / targetFps; // ms

        this.width = width;
        this.height = height;

        this.imageData = null;

        if (video) {
            this.video = video;
        }
        else {
            this.video = document.createElement("video");
            this.video.setAttribute("autoplay", "");
            this.video.setAttribute("muted", "");
            this.video.setAttribute("playsinline", "");
        }

        this.grayScaleMedia = new GrayScaleMedia(this.video, this.width, this.height);
    }

    _onInit(source) {
        function startTick() {
            setInterval(this.tick.bind(this), this.fpsInterval);
        }

        this.glitterModule = new GlitterModule(this.code, startTick.bind(this));
        const initEvent = new CustomEvent("onGlitterInit", { detail: { source: source } });
        document.dispatchEvent(initEvent);
    }

    start() {
        this.grayScaleMedia.requestStream()
            .then(source => {
                this._onInit(source);
            })
            .catch(err => {
                console.warn("ERROR: " + err);
            });
    }

    tick() {
        const start = performance.now();

        this.imageData = this.grayScaleMedia.getPixels();

        const mid = performance.now();

        const quads = this.glitterModule.track(this.imageData, this.width, this.height);

        const end = performance.now();

        if (end-start > this.fpsInterval) {
            console.log("getPixels:", mid-start, "quads:", end-mid, "total:", end-start);
        }

        if (quads) {
            const tagEvent = new CustomEvent("onGlitterTagsFound", { detail: { tags: quads } });
            document.dispatchEvent(tagEvent);
        }
    }
}
