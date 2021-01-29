#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>

#include <emscripten/emscripten.h>

#include "apriltag.h"
#include "tag36h11.h"

#include "common/getopt.h"
#include "common/image_u8.h"
#include "common/image_u8x4.h"
#include "common/pjpeg.h"
#include "common/zarray.h"

#include "lightanchor.h"
#include "lightanchor_detector.h"

apriltag_family_t *tf = NULL;
apriltag_detector_t *td = NULL;
lightanchor_detector_t *ld = NULL;

EMSCRIPTEN_KEEPALIVE
int init(uint8_t code) {
    tf = tag36h11_create();
    if (tf == NULL)
        return 1;

    td = apriltag_detector_create();
    if (td == NULL)
        return 1;

    apriltag_detector_add_family(td, tf);

    ld = lightanchor_detector_create(code);
    if (ld == NULL)
        return 1;

    td->debug = 0;
    td->nthreads = 1;
    td->quad_decimate = 1.0;

    return 0;
}

EMSCRIPTEN_KEEPALIVE
int set_detector_options(uint8_t range_thres, float quad_sigma, int refine_edges, int decode_sharpening, int min_white_black_diff) {
    if (td == NULL)
        return 1;

    ld->range_thres = range_thres;
    td->quad_sigma = quad_sigma;
    td->refine_edges = refine_edges;
    td->decode_sharpening = decode_sharpening;
    td->qtp.min_white_black_diff = min_white_black_diff;

    return 0;
}

EMSCRIPTEN_KEEPALIVE
int set_quad_decimate(float quad_decimate) {
    if (td == NULL)
        return 1;

    td->quad_decimate = quad_decimate;

    return 0;
}

EMSCRIPTEN_KEEPALIVE
int save_grayscale(uint8_t pixels[], uint8_t gray[], int cols, int rows) {
    const int len = cols*rows*4;
    for (int i = 0, j = 0; i < len; i+=4, j++) {
        gray[j] = pixels[i];
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE
double *detect_tags(uint8_t gray[], int cols, int rows) {
    double *output;

    image_u8_t im = {
        .width = cols,
        .height = rows,
        .stride = cols,
        .buf = gray
    };

    zarray_t *quads = detect_quads(td, &im);
    zarray_t *lightanchors = decode_tags(ld, quads, &im);

    const int num_tags = zarray_size(lightanchors);
    const int len = 1 + 10*num_tags; // len + 8 quad pts + 2 center pts
    output = calloc(len, sizeof(double));
    output[0] = num_tags;

    for (int i = 0; i < zarray_size(lightanchors); i++) {
        struct lightanchor *la;
        zarray_get_volatile(lightanchors, i, &la);

        // adjust centers of pixels so that they correspond to the
        // original full-resolution image.
        if (td->quad_decimate > 1)
        {
            for (int j = 0; j < 4; j++)
            {
                la->p[j][0] = (la->p[j][0] - 0.5) * td->quad_decimate + 0.5;
                la->p[j][1] = (la->p[j][1] - 0.5) * td->quad_decimate + 0.5;
            }
            la->c[0] = (la->c[0] - 0.5) * td->quad_decimate + 0.5;
            la->c[1] = (la->c[1] - 0.5) * td->quad_decimate + 0.5;
        }

        output[10*i+1+0] = la->p[0][0];
        output[10*i+1+1] = la->p[0][1];
        output[10*i+1+2] = la->p[1][0];
        output[10*i+1+3] = la->p[1][1];
        output[10*i+1+4] = la->p[2][0];
        output[10*i+1+5] = la->p[2][1];
        output[10*i+1+6] = la->p[3][0];
        output[10*i+1+7] = la->p[3][1];

        output[10*i+1+8] = la->c[0];
        output[10*i+1+9] = la->c[1];
    }

    return output;
}
