#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>

#include <emscripten/emscripten.h>

#include "apriltag.h"
#include "apriltag_pose.h"
#include "tag36h11.h"

#include "common/getopt.h"
#include "common/image_u8.h"
#include "common/image_u8x4.h"
#include "common/pjpeg.h"
#include "common/zarray.h"

#include "lightanchor.h"
#include "lightanchor_detector.h"

// defaults set for 2020 ipad, with 1280x720 images
// static apriltag_detection_info_t g_det_pose_info = {NULL, 0.15, 636.9118, 360.5100, 997.2827, 997.2827};

static apriltag_family_t *lf = NULL;
static apriltag_detector_t *td = NULL;
static lightanchor_detector_t *ld = NULL;

EMSCRIPTEN_KEEPALIVE
int init()
{
    lf = lightanchor_family_create();
    if (lf == NULL)
        return -1;

    td = apriltag_detector_create();
    if (td == NULL)
        return -1;

    apriltag_detector_add_family(td, lf);

    ld = lightanchor_detector_create();
    if (ld == NULL)
        return -1;

    td->nthreads = 1;
    td->quad_decimate = 1.0;

    td->qtp.max_nmaxima = 5;
    td->qtp.min_cluster_pixels = 0;

    td->qtp.max_line_fit_mse = 10.0;
    td->qtp.cos_critical_rad = cos(10 * M_PI / 180);
    td->qtp.deglitch = 0;

    td->refine_edges = 1;
    td->decode_sharpening = 0.25;

    td->debug = 0;

    ld->ttl_frames = 8;

    ld->thres_dist_shape = 50.0;
    ld->thres_dist_shape_ttl = 20.0;
    ld->thres_dist_center = 25.0;

    return 0;
}

EMSCRIPTEN_KEEPALIVE
int add_code(char code)
{
    return lightanchor_detector_add_code(ld, code);
}

EMSCRIPTEN_KEEPALIVE
int set_detector_options(int range_thres, int min_white_black_diff, int ttl_frames,
                        double thres_dist_shape, double thres_dist_shape_ttl, double thres_dist_center)
{
    ld->range_thres = range_thres;
    td->qtp.min_white_black_diff = min_white_black_diff;
    ld->ttl_frames = ttl_frames;
    ld->thres_dist_shape = thres_dist_shape;
    ld->thres_dist_shape_ttl = thres_dist_shape_ttl;
    ld->thres_dist_center = thres_dist_center;
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int set_quad_decimate(float quad_decimate)
{
    td->quad_decimate = quad_decimate;
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int save_grayscale(uint8_t pixels[], uint8_t gray[], int cols, int rows)
{
    const int len = cols * rows * 4;
    for (int i = 0, j = 0; i < len; i+=4, j++)
    {
        gray[j] = pixels[i];
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int detect_tags(uint8_t gray[], int cols, int rows)
{
    image_u8_t im = {
        .width = cols,
        .height = rows,
        .stride = cols,
        .buf = gray
    };

    // EM_ASM({console.time("detect_quads")});
    zarray_t *quads = detect_quads(td, &im);
    // EM_ASM({console.timeEnd("detect_quads")});

    // EM_ASM({console.time("decode_tags")});
    zarray_t *lightanchors = decode_tags(td, ld, quads, &im);
    // EM_ASM({console.timeEnd("decode_tags")});

    int sz = zarray_size(lightanchors);

    for (int i = 0; i < sz; i++)
    {
        lightanchor_t *la;
        zarray_get(lightanchors, i, &la);

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

        EM_ASM_INT({
            var $a = arguments;
            var i = 0;

            const tag = {};

            tag["code"] = $a[i++];

            tag["corners"] = [];

            const corner0 = {};
            corner0["x"] = $a[i++];
            corner0["y"] = $a[i++];
            tag["corners"].push(corner0);

            const corner1 = {};
            corner1["x"] = $a[i++];
            corner1["y"] = $a[i++];
            tag["corners"].push(corner1);

            const corner2 = {};
            corner2["x"] = $a[i++];
            corner2["y"] = $a[i++];
            tag["corners"].push(corner2);

            const corner3 = {};
            corner3["x"] = $a[i++];
            corner3["y"] = $a[i++];
            tag["corners"].push(corner3);

            const center = {};
            center["x"] = $a[i++];
            center["y"] = $a[i++];
            tag["center"] = center;

            const tagEvent = new CustomEvent("onFlashTagFound", {detail: {tag: tag}});
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

        // apriltag_detection_t det;
        // det.H = matd_copy(la->H);
        // memcpy(det.c, la->c, sizeof(det.c));
        // memcpy(det.p, la->p, sizeof(det.p));
        // g_det_pose_info.det = &det;

        // double err1, err2;
        // apriltag_pose_t pose1, pose2;
        // estimate_tag_pose_orthogonal_iteration(&g_det_pose_info, &err1, &pose1, &err2, &pose2, 50);
    }

    lightanchors_destroy(lightanchors);

    return sz;
}
