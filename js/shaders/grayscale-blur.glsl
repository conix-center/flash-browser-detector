#version 300 es

precision highp int;
precision mediump float;
precision mediump sampler2D;

out vec4 color;
in vec2 texCoord;
uniform vec2 texSize;

uniform sampler2D image;
uniform float kernel[25];

#define pixelAtOffset(img, offset) textureLodOffset((img), texCoord, 0.0f, (offset))
#define S(x,y,k) result += pixelAtOffset(image, ivec2((x),(y))) * kernel[k]

const vec4 g = vec4(0.299f, 0.587f, 0.114f, 0.0f); // vec4(0.333f, 0.333f, 0.333f, 0.0f);

void main(void) {
    vec4 result = vec4(0.0f);

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
    color = vec4(gray, gray, gray, 1.0f);
}
