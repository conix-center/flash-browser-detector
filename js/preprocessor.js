import {GLUtils} from './utils/gl-utils';

export class Preprocessor {
    constructor(width, height, canvas) {
        this.width = width;
        this.height = height;

        this.canvas = canvas ? canvas : document.createElement("canvas");
        this.canvas.width = this.width;
        this.canvas.height = this.height;

        this.kernel = [
            0,0,0,0,0,
            0,0,0,0,0,
            0,0,1,0,0,
            0,0,0,0,0,
            0,0,0,0,0,
        ];

        this.gl = GLUtils.createGL(this.canvas, this.width, this.height);

        const flipProg = require("./shaders/flip-image.glsl");
        const grayProg = require("./shaders/grayscale-blur.glsl");
        const program = GLUtils.createProgram(this.gl, flipProg, grayProg);
        GLUtils.useProgram(this.gl, program);

        this.positionLocation = this.gl.getAttribLocation(program, "position");
        this.gl.vertexAttribPointer(this.positionLocation, 2, this.gl.FLOAT, false, 0, 0);
        this.gl.enableVertexAttribArray(this.positionLocation);

        this.textureSizeLocation = this.gl.getUniformLocation(program, "u_tex_size");
        this.gl.uniform2f(this.textureSizeLocation, this.width, this.height);

        this.kernelLocation = this.gl.getUniformLocation(program, "u_kernel[0]");

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

    setKernelSigma(sigma) {
        // compute new 5x5 gaussian kernel
        var i = 0;
        for (var r = -2; r <= -2; r++) {
            for (var c = -2; c <= -2; c++) {
                var val = Math.exp(-(r*r + c*c) / (2*sigma*sigma));
                this.kernel[i] = val;
                i++;
            }
        }

        var sum = this.kernel.reduce((a, b) => a + b, 0);
        this.kernel = this.kernel.map(function(x) {
            return x / sum;
        });

        this.gl.uniform1fv(this.kernelLocation, this.kernel);
    }

    resize(width, height) {
        this.width = width;
        this.height = height;

        this.canvas.width = this.width;
        this.canvas.height = this.height;
        GLUtils.resize(this.gl);
        this.gl.uniform2f(this.textureSizeLocation, this.width, this.height);

        this.imageData = new Uint8Array(this.width * this.height * 4);
    }

    attachElem(source) {
        this.source = source;
        GLUtils.bindElem(this.gl, this.source);
    }
}
