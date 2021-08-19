export class GLUtils {
    static createGL(canvas, width, height) {
        const gl = canvas.getContext("webgl") || canvas.getContext("experimental-webgl");
        gl.viewport(0, 0, width, height);
        gl.clearColor(0.1, 0.1, 0.1, 1.0);
        gl.clear(gl.COLOR_BUFFER_BIT);
        const vertices = new Float32Array([
            -1, -1,
            -1,  1,
             1,  1,
            -1, -1,
             1,  1,
             1, -1,
        ]);
        const buffer = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
        gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.STATIC_DRAW);
        return gl;
    }

    static resize(gl) {
        gl.viewport(0, 0, gl.drawingBufferWidth, gl.drawingBufferHeight);
    }

    static createShader(gl, type, shaderProg) {
        const shader = gl.createShader(type);

        gl.shaderSource(shader, shaderProg);
        gl.compileShader(shader);

        if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
            alert(gl.getShaderInfoLog(shader));
            return null;
        }

        return shader;
    }

    static createProgram(gl, vertShaderProg, fragShaderProg) {
        const vertShader = GLUtils.createShader(gl, gl.VERTEX_SHADER, vertShaderProg);
        const fragShader = GLUtils.createShader(gl, gl.FRAGMENT_SHADER, fragShaderProg);

        const program = gl.createProgram();
        gl.attachShader(program, vertShader);
        gl.attachShader(program, fragShader);

        gl.linkProgram(program);

        return program;
    }

    static useProgram(gl, program) {
        gl.useProgram(program);
    }

    static createTexture(gl, width, height) {
        const texture = gl.createTexture();

        gl.bindTexture(gl.TEXTURE_2D, texture);

        // if either dimension of image is not a power of 2
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);

        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);

        gl.bindTexture(gl.TEXTURE_2D, null);
        return texture;
    }

    static destroyTexture(gl, texture) {
        gl.deleteTexture(texture);
    }

    static bindTexture(gl, texture) {
        gl.bindTexture(gl.TEXTURE_2D, texture);
        return texture;
    }

    static bindElem(gl, elem) {
        gl.texImage2D(
            gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, elem);
    }

    static updateElem(gl, elem) {
        gl.texSubImage2D(
            gl.TEXTURE_2D, 0, 0, 0, gl.RGBA, gl.UNSIGNED_BYTE, elem);
    }

    static createFramebuffer(gl, texture) {
        const fbo = gl.createFramebuffer();
        gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);
        gl.framebufferTexture2D(
            gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, texture, 0);
        gl.bindFramebuffer(gl.FRAMEBUFFER, null);
        return fbo;
    }

    static checkFramebufferStatus(gl) {
        return gl.checkFramebufferStatus(gl.FRAMEBUFFER) == gl.FRAMEBUFFER_COMPLETE;
    }

    static destroyFramebuffer(gl, fbo) {
        gl.deleteFramebuffer(fbo);
    }

    static draw(gl) {
        gl.drawArrays(gl.TRIANGLES, 0, 6);
    }

    static readPixels(gl, width, height, buffer) {
        gl.readPixels(
            0, 0, width, height, gl.RGBA, gl.UNSIGNED_BYTE, buffer);
    }
}
