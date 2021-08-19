export class Timer {
    constructor(callback, interval) {
        this.callback = callback;
        this.interval = interval;
        this.running = false;
    }

    run() {
        const _this = this;
        const start = performance.now();

        _this.running = true;
        function frame(time) {
            if (!_this.running) return;
            _this.callback(time);
            scheduleFrame(time);
        }

        function scheduleFrame(time) {
            const elapsed = time - start;
            const roundedElapsed = Math.round(elapsed / _this.interval) * _this.interval;
            const targetNext = start + roundedElapsed + _this.interval;
            const delay = targetNext - performance.now();
            setTimeout(() => requestAnimationFrame(frame), delay);
        }

        scheduleFrame(start);
    }

    stop() {
        this.running = false;
    }
}
