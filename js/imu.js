import Swal from "sweetalert2";

export class DeviceIMU {
    constructor() {
        this.screenOrientation = 0;
        this.deviceOrientation = {};

        this.updateScreenOrientation();
    }

    init() {
        if (DeviceOrientationEvent !== undefined && typeof DeviceOrientationEvent.requestPermission === "function") {
            Swal.fire({
                title: "FLASH requires access to your device orientation and motion sensors.",
                icon: "warning",
                showConfirmButton: true,
                showCancelButton: true,
                confirmButtonText: "Allow",
                cancelButtonText: "Deny",
                confirmButtonColor: "#3085d6",
                cancelButtonColor: "#d33",
                reverseButtons: true
            })
                .then((result) => {
                    if (result.isConfirmed) {
                        this.requestPermission();
                    }
                });
        }
        else {
            window.addEventListener("deviceorientation", this.updateDeviceOrientation.bind(this));
            window.addEventListener("orientationchange", this.updateScreenOrientation.bind(this));
        }
    }

    requestPermission() {
        DeviceOrientationEvent.requestPermission()
            .then((permissionState) => {
                if (permissionState == "granted") {
                    window.addEventListener("deviceorientation", this.updateDeviceOrientation.bind(this));
                    window.addEventListener("orientationchange", this.updateScreenOrientation.bind(this));
                }
            })
            .catch((err) => {
                console.error(err);
            });
    }

    updateScreenOrientation(e) {
        this.screenOrientation = window.orientation || 0;
    }

    updateDeviceOrientation(e) {
        this.deviceOrientation = e;
    }
}
