export class GlitterSource {
    constructor(width, height) {
        this.width = width;
        this.height = height;

        this.video = document.createElement("video");
        this.video.setAttribute("autoplay", "");
        this.video.setAttribute("muted", "");
        this.video.setAttribute("playsinline", "");
        this.video.style.width = this.width + "px";
        this.video.style.height = this.height + "px";

        this.video.style.position = "absolute";
        this.video.style.top = "0px";
        this.video.style.left = "0px";
        this.video.style.zIndex = "-1";
    }

    init() {
        return new Promise((resolve, reject) => {
            if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia)
                return reject();

            navigator.mediaDevices.getUserMedia({
                audio: false,
                video: {
                    facingMode: "environment",
                    width: { ideal: this.width },
                    height: { ideal: this.height },
                }
            })
            .then((stream) => {
                this.video.srcObject = stream;
                this.video.onloadedmetadata = (e) => {
                    this.video.play();
                    resolve(this.video);
                };
            })
            .catch((err) => {
                reject(err);
            });
        });
    }
}
