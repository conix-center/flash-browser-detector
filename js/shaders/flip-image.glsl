attribute vec2 position;
varying vec2 tex_coords;

void main(void) {
    tex_coords = (position + 1.0) / 2.0;
    gl_Position = vec4(position, 0.0, 1.0);
}
