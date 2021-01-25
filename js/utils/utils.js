export class Utils {
    static isMobile () {
        return /Android|mobile|iPad|iPhone/i.test(navigator.userAgent);
    }

    static round2(num) {
        return Math.round((num + Number.EPSILON) * 100) / 100;
    }
}
