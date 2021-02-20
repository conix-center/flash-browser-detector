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
int init() {
    tf = tag36h11_create();
    if (tf == NULL)
        return 1;

    td = apriltag_detector_create();
    if (td == NULL)
        return 1;

    apriltag_detector_add_family(td, tf);

    ld = lightanchor_detector_create();
    if (ld == NULL)
        return 1;

    td->nthreads = 1;
    td->quad_decimate = 1.0;

    td->qtp.max_nmaxima = 10;
    td->qtp.min_cluster_pixels = 5;

    td->qtp.max_line_fit_mse = 10.0;
    td->qtp.cos_critical_rad = cos(10 * M_PI / 180);
    td->qtp.deglitch = 0;


    td->debug = 0;

    return 0;
}

EMSCRIPTEN_KEEPALIVE
int add_code(char code) {
    return lightanchor_detector_add_code(ld, code);
}

EMSCRIPTEN_KEEPALIVE
int set_detector_options(int range_thres, int refine_edges, int min_white_black_diff) {
    if (td == NULL)
        return 1;

    ld->range_thres = range_thres;
    td->refine_edges = refine_edges;
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
int detect_tags(uint8_t gray[], int cols, int rows) {
    image_u8_t im = {
        .width = cols,
        .height = rows,
        .stride = cols,
        .buf = gray
    };

    zarray_t *quads = detect_quads(td, &im);
    zarray_t *lightanchors = decode_tags(td, ld, quads, &im);

    int sz = zarray_size(lightanchors);

    for (int i = 0; i < sz; i++) {
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

        EM_ASM({
            const tag = {};

            tag["code"] = $0;

            tag["corners"] = [];

            const corner0 = {};
            corner0["x"] = $1;
            corner0["y"] = $2;
            tag["corners"].push(corner0);

            const corner1 = {};
            corner1["x"] = $3;
            corner1["y"] = $4;
            tag["corners"].push(corner1);

            const corner2 = {};
            corner2["x"] = $5;
            corner2["y"] = $6;
            tag["corners"].push(corner2);

            const corner3 = {};
            corner3["x"] = $7;
            corner3["y"] = $8;
            tag["corners"].push(corner3);

            const center = {};
            center["x"] = $9;
            center["y"] = $10;
            tag["center"] = center;

            const tagEvent = new CustomEvent("onGlitterTagFound", {detail: {tag: tag}});
            var scope;
            if ('function' === typeof importScripts)
                scope = self;
            else
                scope = window;
            scope.dispatchEvent(tagEvent);
        },
            la->match_code,
            la->p[0][0],
            la->p[0][1],
            la->p[1][0],
            la->p[1][1],
            la->p[2][0],
            la->p[2][1],
            la->p[3][0],
            la->p[3][1],
            la->c[0],
            la->c[1]
        );
    }
    lightanchors_destroy(lightanchors);

    return sz;
}
