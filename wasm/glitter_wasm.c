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

#include "lightanchor_detector.h"

apriltag_family_t *tf = NULL;
apriltag_detector_t *td = NULL;
lightanchor_detector_t *ld = NULL;

EMSCRIPTEN_KEEPALIVE
int init() {
    tf = tag36h11_create();

    td = apriltag_detector_create();
    apriltag_detector_add_family(td, tf);

    td->quad_decimate = 2.0;
    td->quad_sigma = 1.0;
    td->nthreads = 1;
    td->debug = 0;
    td->refine_edges = 1;

    ld = lightanchor_detector_create(0xaf);

    return 0;
}

EMSCRIPTEN_KEEPALIVE
double *track(uint8_t frame[], size_t cols, size_t rows) {
    double *output;

    image_u8_t im = {
        .width = cols,
        .height = rows,
        .stride = cols,
        .buf = frame
    };

    zarray_t *quads = detect_quads(td, &im);
    zarray_t *lightanchors = decode_tags(ld, quads, &im);

    output = malloc(6*zarray_size(lightanchors)*sizeof(double)); // 4 quad pts + 2 center

    for (int i = 0; i < zarray_size(lightanchors); i++) {
        lightanchor_t *lightanchor;
        zarray_get_volatile(lightanchors, i, &lightanchor);

        output[6*i]   = lightanchor->p[0][0];
        output[6*i+1] = lightanchor->p[0][1];
        output[6*i+2] = lightanchor->p[1][0];
        output[6*i+3] = lightanchor->p[1][1];

        output[6*i+4] = lightanchor->c[0];
        output[6*i+5] = lightanchor->c[1];
    }

    return output;
}
