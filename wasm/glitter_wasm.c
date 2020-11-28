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
int init(uint8_t code) {
    tf = tag36h11_create();

    td = apriltag_detector_create();
    apriltag_detector_add_family(td, tf);

    td->quad_decimate = 2.0;
    td->quad_sigma = 0.5;
    td->nthreads = 1;
    td->debug = 0;
    td->refine_edges = 1;

    ld = lightanchor_detector_create(code);

    printf("Ready!\n");

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

    const int size = (1+10*zarray_size(quads))*sizeof(double); // len + 4 quad pts + 2 center
    output = calloc(1, size);
    output[0] = zarray_size(lightanchors);

    for (int i = 0; i < zarray_size(lightanchors); i++) {
        struct lightanchor *la;
        zarray_get_volatile(lightanchors, i, &la);

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
