precision highp int;
precision mediump float;

uniform sampler2D image;
varying vec2 tex_coords;

uniform vec2 tex_size;
uniform float kernel[25];

#define S(x,y,k) result += texture2D(image, tex_coords + pixel * vec2(x,y)) * kernel[k]

const vec4 g = vec4(0.299, 0.587, 0.114, 0.0); // vec4(0.333, 0.333, 0.333, 0.0);

void main(void) {
    vec2 pixel = vec2(1.0, 1.0) / tex_size;
    vec4 result = vec4(0.0, 0.0, 0.0, 0.0);

    S(-2,-2, 24);
    S(-2,-1, 23);
    S(-2, 0, 22);
    S(-2, 1, 21);
    S(-2, 2, 20);
    S(-1,-2, 19);
    S(-1,-1, 18);
    S(-1, 0, 17);
    S(-1, 1, 16);
    S(-1, 2, 15);
    S( 0,-2, 14);
    S( 0,-1, 13);
    S( 0, 0, 12);
    S( 0, 1, 11);
    S( 0, 2, 10);
    S( 1,-2, 9);
    S( 1,-1, 8);
    S( 1, 0, 7);
    S( 1, 1, 6);
    S( 1, 2, 5);
    S( 2,-2, 4);
    S( 2,-1, 3);
    S( 2, 0, 2);
    S( 2, 1, 1);
    S( 2, 2, 0);

    float gray = dot(result, g);
    gl_FragColor = vec4(gray, gray, gray, 1.0);
}
