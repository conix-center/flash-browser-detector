#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

uniform sampler2D u_image;
varying vec2 tex_coords;

uniform vec2 u_tex_size;
uniform float u_kernel[25];

const vec4 g = vec4(0.333, 0.333, 0.333, 0.0); // vec4(0.299, 0.587, 0.114, 0.0);

void main(void) {
    vec2 pixel = vec2(1.0, 1.0) / u_tex_size;
    vec4 filtered =
        texture2D(u_image, tex_coords + pixel * vec2(-2, -2)) * u_kernel[0]  +
        texture2D(u_image, tex_coords + pixel * vec2(-1, -2)) * u_kernel[1]  +
        texture2D(u_image, tex_coords + pixel * vec2( 0, -2)) * u_kernel[2]  +
        texture2D(u_image, tex_coords + pixel * vec2( 1, -2)) * u_kernel[3]  +
        texture2D(u_image, tex_coords + pixel * vec2( 2, -2)) * u_kernel[4]  +
        texture2D(u_image, tex_coords + pixel * vec2(-2, -1)) * u_kernel[5]  +
        texture2D(u_image, tex_coords + pixel * vec2(-1, -1)) * u_kernel[6]  +
        texture2D(u_image, tex_coords + pixel * vec2( 0, -1)) * u_kernel[7]  +
        texture2D(u_image, tex_coords + pixel * vec2( 1, -1)) * u_kernel[8]  +
        texture2D(u_image, tex_coords + pixel * vec2( 2, -1)) * u_kernel[9]  +
        texture2D(u_image, tex_coords + pixel * vec2(-2,  0)) * u_kernel[10] +
        texture2D(u_image, tex_coords + pixel * vec2(-1,  0)) * u_kernel[11] +
        texture2D(u_image, tex_coords + pixel * vec2( 0,  0)) * u_kernel[12] +
        texture2D(u_image, tex_coords + pixel * vec2( 1,  0)) * u_kernel[13] +
        texture2D(u_image, tex_coords + pixel * vec2( 2,  0)) * u_kernel[14] +
        texture2D(u_image, tex_coords + pixel * vec2(-2,  1)) * u_kernel[15] +
        texture2D(u_image, tex_coords + pixel * vec2(-1,  1)) * u_kernel[16] +
        texture2D(u_image, tex_coords + pixel * vec2( 0,  1)) * u_kernel[17] +
        texture2D(u_image, tex_coords + pixel * vec2( 1,  1)) * u_kernel[18] +
        texture2D(u_image, tex_coords + pixel * vec2( 2,  1)) * u_kernel[19] +
        texture2D(u_image, tex_coords + pixel * vec2(-2,  2)) * u_kernel[20] +
        texture2D(u_image, tex_coords + pixel * vec2(-1,  2)) * u_kernel[21] +
        texture2D(u_image, tex_coords + pixel * vec2( 0,  2)) * u_kernel[22] +
        texture2D(u_image, tex_coords + pixel * vec2( 1,  2)) * u_kernel[23] +
        texture2D(u_image, tex_coords + pixel * vec2( 2,  2)) * u_kernel[24] ;
    float gray = dot(filtered, g);
    gl_FragColor = vec4(gray, gray, gray, 1.0);
}
