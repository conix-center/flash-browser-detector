import Swal from "sweetalert2";

export class DeviceIMU {
    constructor() {
        this.orientationActive = false;
        this.motionActive = false;

        this.orientation = null;
        this.acceleration = null;
        this.accelerationIncludingGravity = null;
        this.rotationRate = null;

        Swal.fire({
            title: "GLITTER requires access to your device orientation and motion sensors.",
            icon: "warning",
            showConfirmButton: true,
            showCancelButton: true,
            confirmButtonText: "Allow",
            cancelButtonText: "Deny",
            confirmButtonColor: "#3085d6",
            cancelButtonColor: "#d33",
        })
            .then((result) => {
                if (result.isConfirmed) {
                    this.requestPermission();
                }
            });
    }

    requestPermission() {
        if (DeviceOrientationEvent && typeof(DeviceOrientationEvent.requestPermission) === "function") {
            DeviceOrientationEvent.requestPermission()
                .then((permissionState) => {
                    if (permissionState == "granted") {
                        this.orientationActive = true;
                        window.addEventListener("deviceorientation", this.updateOrientation.bind(this));
                    }
                })
                .catch((err) => {
                    console.error(err);
                });
        }

        if (DeviceMotionEvent && typeof(DeviceMotionEvent.requestPermission) === "function") {
            DeviceMotionEvent.requestPermission()
                .then((permissionState) => {
                    if (permissionState == "granted") {
                        this.motionActive = true;
                        window.addEventListener("devicemotion", this.updateMotion.bind(this));
                    }
                })
                .catch((err) => {
                    console.error(err);
                });
        }
    }

    updateOrientation(e) {
        this.orientation = e;
    }

    updateMotion(e) {
        this.acceleration = e.acceleration;
        this.accelerationIncludingGravity = e.accelerationIncludingGravity;
        this.rotationRate = e.rotationRate;
    }
}
