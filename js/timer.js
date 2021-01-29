export class Timer {
    constructor(callback, interval) {
        this.callback = callback;
        this.interval = interval;
        this.running = false;
    }

    run() {
        this.running = true;
        this.totalDt = 0; this.iters = 0;

        this.expected = Date.now() + this.interval;
        setTimeout(tick.bind(this), this.interval);
        function tick() {
            if (!this.running) return;
            const dt = Date.now() - this.expected; // calculate drift
            if (dt > this.interval) { // adjust for errors
                this.expected += this.interval * Math.floor(dt / this.interval);
            }

            const startCompute = Date.now();
            this.totalDt += Math.abs(dt); this.iters++;

            this.callback();

            this.expected += this.interval;
            const computationTime = Date.now() - startCompute;
            setTimeout(tick.bind(this), Math.max(0, this.interval - dt - computationTime)); // take into account drift
        }
    }

    stop() {
        this.running = false;
    }

    getError() {
        if (this.iters > 0) {
            return Math.abs(this.totalDt / this.iters);
        }
        else {
            return -1;
        }
    }
}
