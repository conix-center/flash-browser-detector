import {GLUtils} from './utils/gl-utils';

export class GrayScale {
    constructor(width, height, canvas) {
        this.width = width;
        this.height = height;

        this.canvas = canvas ? canvas : document.createElement("canvas");
        this.canvas.width = this.width;
        this.canvas.height = this.height;

        this.gl = GLUtils.createGL(this.canvas, this.width, this.height);

        const flipProg = require("./shaders/flip-image.glsl");
        const grayProg = require("./shaders/grayscale.glsl");
        const program = GLUtils.createProgram(this.gl, flipProg, grayProg);
        GLUtils.useProgram(this.gl, program);

        const positionLocation = this.gl.getAttribLocation(program, "position");
        this.gl.vertexAttribPointer(positionLocation, 2, this.gl.FLOAT, false, 0, 0);
        this.gl.enableVertexAttribArray(positionLocation);

        const flipYLocation = this.gl.getUniformLocation(program, "flipY");
        this.gl.uniform1f(flipYLocation, -1); // flip image

        this.texture = GLUtils.createTexture(this.gl, this.width, this.height);
        GLUtils.bindTexture(this.gl, this.texture);

        this.imageData = new Uint8Array(this.width * this.height * 4);
    }

    getPixels() {
        if (this.source) {
            GLUtils.bindElem(this.gl, this.source);
            GLUtils.draw(this.gl);
            GLUtils.readPixels(this.gl, this.width, this.height, this.imageData);
            return this.imageData;
        }
        else {
            return null;
        }
    }

    resize(width, height) {
        this.width = width;
        this.height = height;

        this.canvas.width = this.width;
        this.canvas.height = this.height;
        GLUtils.resize(this.gl);

        this.imageData = new Uint8Array(this.width * this.height * 4);
    }

    attachElem(source) {
        this.source = source;
        GLUtils.bindElem(this.gl, this.source);
    }
}
