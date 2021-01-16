(function webpackUniversalModuleDefinition(root, factory) {
	if(typeof exports === 'object' && typeof module === 'object')
		module.exports = factory();
	else if(typeof define === 'function' && define.amd)
		define([], factory);
	else if(typeof exports === 'object')
		exports["Glitter"] = factory();
	else
		root["Glitter"] = factory();
})(typeof self !== 'undefined' ? self : this, function() {
return /******/ (function(modules) { // webpackBootstrap
/******/ 	// The module cache
/******/ 	var installedModules = {};
/******/
/******/ 	// The require function
/******/ 	function __webpack_require__(moduleId) {
/******/
/******/ 		// Check if module is in cache
/******/ 		if(installedModules[moduleId]) {
/******/ 			return installedModules[moduleId].exports;
/******/ 		}
/******/ 		// Create a new module (and put it into the cache)
/******/ 		var module = installedModules[moduleId] = {
/******/ 			i: moduleId,
/******/ 			l: false,
/******/ 			exports: {}
/******/ 		};
/******/
/******/ 		// Execute the module function
/******/ 		modules[moduleId].call(module.exports, module, module.exports, __webpack_require__);
/******/
/******/ 		// Flag the module as loaded
/******/ 		module.l = true;
/******/
/******/ 		// Return the exports of the module
/******/ 		return module.exports;
/******/ 	}
/******/
/******/
/******/ 	// expose the modules object (__webpack_modules__)
/******/ 	__webpack_require__.m = modules;
/******/
/******/ 	// expose the module cache
/******/ 	__webpack_require__.c = installedModules;
/******/
/******/ 	// define getter function for harmony exports
/******/ 	__webpack_require__.d = function(exports, name, getter) {
/******/ 		if(!__webpack_require__.o(exports, name)) {
/******/ 			Object.defineProperty(exports, name, { enumerable: true, get: getter });
/******/ 		}
/******/ 	};
/******/
/******/ 	// define __esModule on exports
/******/ 	__webpack_require__.r = function(exports) {
/******/ 		if(typeof Symbol !== 'undefined' && Symbol.toStringTag) {
/******/ 			Object.defineProperty(exports, Symbol.toStringTag, { value: 'Module' });
/******/ 		}
/******/ 		Object.defineProperty(exports, '__esModule', { value: true });
/******/ 	};
/******/
/******/ 	// create a fake namespace object
/******/ 	// mode & 1: value is a module id, require it
/******/ 	// mode & 2: merge all properties of value into the ns
/******/ 	// mode & 4: return value when already ns object
/******/ 	// mode & 8|1: behave like require
/******/ 	__webpack_require__.t = function(value, mode) {
/******/ 		if(mode & 1) value = __webpack_require__(value);
/******/ 		if(mode & 8) return value;
/******/ 		if((mode & 4) && typeof value === 'object' && value && value.__esModule) return value;
/******/ 		var ns = Object.create(null);
/******/ 		__webpack_require__.r(ns);
/******/ 		Object.defineProperty(ns, 'default', { enumerable: true, value: value });
/******/ 		if(mode & 2 && typeof value != 'string') for(var key in value) __webpack_require__.d(ns, key, function(key) { return value[key]; }.bind(null, key));
/******/ 		return ns;
/******/ 	};
/******/
/******/ 	// getDefaultExport function for compatibility with non-harmony modules
/******/ 	__webpack_require__.n = function(module) {
/******/ 		var getter = module && module.__esModule ?
/******/ 			function getDefault() { return module['default']; } :
/******/ 			function getModuleExports() { return module; };
/******/ 		__webpack_require__.d(getter, 'a', getter);
/******/ 		return getter;
/******/ 	};
/******/
/******/ 	// Object.prototype.hasOwnProperty.call
/******/ 	__webpack_require__.o = function(object, property) { return Object.prototype.hasOwnProperty.call(object, property); };
/******/
/******/ 	// __webpack_public_path__
/******/ 	__webpack_require__.p = "";
/******/
/******/
/******/ 	// Load entry module and return exports
/******/ 	return __webpack_require__(__webpack_require__.s = "./js/index.js");
/******/ })
/************************************************************************/
/******/ ({

/***/ "./js/glitter-detector.js":
/*!********************************!*\
  !*** ./js/glitter-detector.js ***!
  \********************************/
/*! exports provided: GlitterDetector */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "GlitterDetector", function() { return GlitterDetector; });
/* harmony import */ var _grayscale__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! ./grayscale */ "./js/grayscale.js");
/* harmony import */ var _glitter_module__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ./glitter-module */ "./js/glitter-module.js");


class GlitterDetector {
  constructor(code, targetFps, width, height, video) {
    this.code = code;
    this.fpsInterval = 1000 / targetFps; // ms

    this.width = width;
    this.height = height;
    this.imageData = null;

    if (video) {
      this.video = video;
    } else {
      this.video = document.createElement("video");
      this.video.setAttribute("autoplay", "");
      this.video.setAttribute("muted", "");
      this.video.setAttribute("playsinline", "");
    }

    this.grayScaleMedia = new _grayscale__WEBPACK_IMPORTED_MODULE_0__["GrayScaleMedia"](this.video, this.width, this.height);
  }

  _onInit(source) {
    function startTick() {
      setInterval(this.tick.bind(this), this.fpsInterval);
    }

    this.glitterModule = new _glitter_module__WEBPACK_IMPORTED_MODULE_1__["GlitterModule"](this.code, startTick.bind(this));
    const initEvent = new CustomEvent("onGlitterInit", {
      detail: {
        source: source
      }
    });
    document.dispatchEvent(initEvent);
  }

  start() {
    this.grayScaleMedia.requestStream().then(source => {
      this._onInit(source);
    }).catch(err => {
      console.warn("ERROR: " + err);
    });
  }

  tick() {
    const start = performance.now();
    this.imageData = this.grayScaleMedia.getPixels();
    const mid = performance.now();
    const quads = this.glitterModule.track(this.imageData, this.width, this.height);
    const end = performance.now();

    if (end - start > this.fpsInterval) {
      console.log("getPixels:", mid - start, "quads:", end - mid, "total:", end - start);
    }

    if (quads) {
      const tagEvent = new CustomEvent("onGlitterTagsFound", {
        detail: {
          tags: quads
        }
      });
      document.dispatchEvent(tagEvent);
    }
  }

}

/***/ }),

/***/ "./js/glitter-module.js":
/*!******************************!*\
  !*** ./js/glitter-module.js ***!
  \******************************/
/*! exports provided: GlitterModule */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "GlitterModule", function() { return GlitterModule; });
class GlitterModule {
  constructor(code, callback) {
    let _this = this;

    this.ready = false;
    this.shouldTrack = false;
    this.validPoints = false;
    this.code = code;
    GlitterWASM().then(function (Module) {
      console.log("GLITTER WASM module loaded.");

      _this.onWasmInit(Module);

      if (callback) callback();
    });
  }

  onWasmInit(Module) {
    this._Module = Module;
    this._init = Module.cwrap("init", "number", ["number"]);
    this._track = this._Module.cwrap("track", "number", ["number", "number", "number"]);
    this.ready = this._init(this.code) == 0;
  }

  track(im_arr, width, height) {
    let quads = [];
    if (!this.ready) return quads;

    const im_ptr = this._Module._malloc(im_arr.length);

    this._Module.HEAPU8.set(im_arr, im_ptr);

    const ptr = this._track(im_ptr, width, height);

    const ptrF64 = ptr / Float64Array.BYTES_PER_ELEMENT;

    const numQuads = this._Module.getValue(ptr, "double"); // console.log("numQuads = ", numQuads);


    for (var i = 0; i < numQuads; i++) {
      var q = {
        p00: this._Module.HEAPF64[ptrF64 + 10 * i + 1 + 0],
        p01: this._Module.HEAPF64[ptrF64 + 10 * i + 1 + 1],
        p10: this._Module.HEAPF64[ptrF64 + 10 * i + 1 + 2],
        p11: this._Module.HEAPF64[ptrF64 + 10 * i + 1 + 3],
        p20: this._Module.HEAPF64[ptrF64 + 10 * i + 1 + 4],
        p21: this._Module.HEAPF64[ptrF64 + 10 * i + 1 + 5],
        p30: this._Module.HEAPF64[ptrF64 + 10 * i + 1 + 6],
        p31: this._Module.HEAPF64[ptrF64 + 10 * i + 1 + 7],
        c0: this._Module.HEAPF64[ptrF64 + 10 * i + 1 + 8],
        c1: this._Module.HEAPF64[ptrF64 + 10 * i + 1 + 9]
      };
      quads.push(q);
    }

    this._Module._free(ptr);

    this._Module._free(im_ptr);

    return quads;
  }

}

/***/ }),

/***/ "./js/grayscale.js":
/*!*************************!*\
  !*** ./js/grayscale.js ***!
  \*************************/
/*! exports provided: GrayScaleMedia */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "GrayScaleMedia", function() { return GrayScaleMedia; });
/* harmony import */ var _utils_utils__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! ./utils/utils */ "./js/utils/utils.js");
/* harmony import */ var _utils_gl_utils__WEBPACK_IMPORTED_MODULE_1__ = __webpack_require__(/*! ./utils/gl-utils */ "./js/utils/gl-utils.js");


class GrayScaleMedia {
  constructor(source, width, height, canvas) {
    this.source = source;
    this.width = width;
    this.height = height;
    this.canvas = canvas ? canvas : document.createElement("canvas");
    this.canvas.width = this.width;
    this.canvas.height = this.height;
    this.gl = _utils_gl_utils__WEBPACK_IMPORTED_MODULE_1__["GLUtils"].createGL(this.canvas, this.width, this.height);

    const flipProg = __webpack_require__(/*! ./shaders/flip-image.glsl */ "./js/shaders/flip-image.glsl");

    const grayProg = __webpack_require__(/*! ./shaders/grayscale.glsl */ "./js/shaders/grayscale.glsl");

    const program = _utils_gl_utils__WEBPACK_IMPORTED_MODULE_1__["GLUtils"].createProgram(this.gl, flipProg, grayProg);
    _utils_gl_utils__WEBPACK_IMPORTED_MODULE_1__["GLUtils"].useProgram(this.gl, program);
    const positionLocation = this.gl.getAttribLocation(program, "position");
    this.gl.vertexAttribPointer(positionLocation, 2, this.gl.FLOAT, false, 0, 0);
    this.gl.enableVertexAttribArray(positionLocation);
    const flipLocation = this.gl.getUniformLocation(program, "flipY");
    this.gl.uniform1f(flipLocation, -1); // flip image

    this.texture = _utils_gl_utils__WEBPACK_IMPORTED_MODULE_1__["GLUtils"].createTexture(this.gl, this.width, this.height);
    _utils_gl_utils__WEBPACK_IMPORTED_MODULE_1__["GLUtils"].bindTexture(this.gl, this.texture);
    this.glReady = true;
    this.pixelBuf = new Uint8ClampedArray(this.width * this.height * 4);
    this.grayBuf = new Uint8ClampedArray(this.width * this.height);
  }

  getPixels() {
    if (!this.glReady) return undefined;
    _utils_gl_utils__WEBPACK_IMPORTED_MODULE_1__["GLUtils"].updateElem(this.gl, this.source);
    _utils_gl_utils__WEBPACK_IMPORTED_MODULE_1__["GLUtils"].draw(this.gl);
    _utils_gl_utils__WEBPACK_IMPORTED_MODULE_1__["GLUtils"].readPixels(this.gl, this.width, this.height, this.pixelBuf);
    let j = 0;

    for (let i = 0; i < this.pixelBuf.length; i += 4) {
      this.grayBuf[j] = this.pixelBuf[i];
      j++;
    }

    return this.grayBuf;
  }

  requestStream() {
    return new Promise((resolve, reject) => {
      if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) return reject(); // Hack for mobile browsers: aspect ratio is flipped.

      var aspect = this.width / this.height;

      if (_utils_utils__WEBPACK_IMPORTED_MODULE_0__["Utils"].isMobile()) {
        aspect = 1 / aspect;
      }

      navigator.mediaDevices.getUserMedia({
        audio: false,
        video: {
          width: {
            ideal: this.width
          },
          height: {
            ideal: this.height
          },
          aspectRatio: {
            ideal: aspect
          },
          facingMode: "environment"
        }
      }).then(stream => {
        this.source.srcObject = stream;

        this.source.onloadedmetadata = e => {
          this.source.play();
          _utils_gl_utils__WEBPACK_IMPORTED_MODULE_1__["GLUtils"].bindElem(this.gl, this.source);
          resolve(this.source);
        };
      }).catch(err => {
        reject(err);
      });
    });
  }

}

/***/ }),

/***/ "./js/index.js":
/*!*********************!*\
  !*** ./js/index.js ***!
  \*********************/
/*! exports provided: GlitterDetector */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony import */ var _glitter_detector__WEBPACK_IMPORTED_MODULE_0__ = __webpack_require__(/*! ./glitter-detector */ "./js/glitter-detector.js");
/* harmony reexport (safe) */ __webpack_require__.d(__webpack_exports__, "GlitterDetector", function() { return _glitter_detector__WEBPACK_IMPORTED_MODULE_0__["GlitterDetector"]; });




/***/ }),

/***/ "./js/shaders/flip-image.glsl":
/*!************************************!*\
  !*** ./js/shaders/flip-image.glsl ***!
  \************************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "attribute vec2 position;\nvarying vec2 tex_coords;\nuniform float flipY;\nvoid main(void) {\ntex_coords = (position + 1.0) / 2.0;\ntex_coords.y = 1.0 - tex_coords.y;\ngl_Position = vec4(position * vec2(1, flipY), 0.0, 1.0);\n}"

/***/ }),

/***/ "./js/shaders/grayscale.glsl":
/*!***********************************!*\
  !*** ./js/shaders/grayscale.glsl ***!
  \***********************************/
/*! no static exports found */
/***/ (function(module, exports) {

module.exports = "precision highp float;\nuniform sampler2D u_image;\nvarying vec2 tex_coords;\nconst vec3 g = vec3(0.299, 0.587, 0.114);\nvoid main(void) {\nvec4 color = texture2D(u_image, tex_coords);\nfloat gray = dot(color.rgb, g);\ngl_FragColor = vec4(vec3(gray), 1.0);\n}"

/***/ }),

/***/ "./js/utils/gl-utils.js":
/*!******************************!*\
  !*** ./js/utils/gl-utils.js ***!
  \******************************/
/*! exports provided: GLUtils */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "GLUtils", function() { return GLUtils; });
class GLUtils {
  static createGL(canvas, width, height) {
    const gl = canvas.getContext("webgl") || canvas.getContext("experimental-webgl");
    gl.viewport(0, 0, width, height);
    gl.clearColor(0.1, 0.1, 0.1, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT);
    const vertices = new Float32Array([-1, -1, -1, 1, 1, 1, -1, -1, 1, 1, 1, -1]);
    const buffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
    gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.STATIC_DRAW);
    return gl;
  }

  static createShader(gl, type, shaderProg) {
    const shader = gl.createShader(type);
    gl.shaderSource(shader, shaderProg);
    gl.compileShader(shader);
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
    gl.bindTexture(gl.TEXTURE_2D, texture); // if either dimension of image is not a power of 2

    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
    gl.bindTexture(gl.TEXTURE_2D, null);
    return texture;
  }

  static bindTexture(gl, texture) {
    gl.bindTexture(gl.TEXTURE_2D, texture);
    return texture;
  }

  static bindElem(gl, elem) {
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, elem);
  }

  static updateElem(gl, elem) {
    gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, gl.RGBA, gl.UNSIGNED_BYTE, elem);
  }

  static destroyTexture(gl, texture) {
    gl.deleteTexture(texture);
  }

  static createFramebuffer(gl, texture) {
    const fbo = gl.createFramebuffer();
    gl.bindFramebuffer(gl.FRAMEBUFFER, fbo);
    gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, texture, 0);
    return fbo;
  }

  static destroyFramebuffer(gl, fbo) {
    gl.deleteFramebuffer(fbo);
  }

  static draw(gl) {
    gl.drawArrays(gl.TRIANGLES, 0, 6);
  }

  static readPixels(gl, width, height, buffer) {
    gl.readPixels(0, 0, width, height, gl.RGBA, gl.UNSIGNED_BYTE, buffer);
  }

}

/***/ }),

/***/ "./js/utils/utils.js":
/*!***************************!*\
  !*** ./js/utils/utils.js ***!
  \***************************/
/*! exports provided: Utils */
/***/ (function(module, __webpack_exports__, __webpack_require__) {

"use strict";
__webpack_require__.r(__webpack_exports__);
/* harmony export (binding) */ __webpack_require__.d(__webpack_exports__, "Utils", function() { return Utils; });
class Utils {
  static isMobile() {
    return /Android|mobile|iPad|iPhone/i.test(navigator.userAgent);
  }

}

/***/ })

/******/ });
});
//# sourceMappingURL=glitter.js.map