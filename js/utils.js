export default class Utils {
    static isMobile () {
        return /Android|mobile|iPad|iPhone/i.test(navigator.userAgent);
    }
}
