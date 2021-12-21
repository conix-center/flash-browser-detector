#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>

#include <emscripten/emscripten.h>
#include <emscripten/bind.h>

extern "C" {
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
}

using namespace emscripten;

// defaults set for 2020 ipad, with 1280x720 images
static apriltag_detection_info_t pose_info = {.tagsize=0.15, .fx=997.2827, .fy=997.2827, .cx=636.9118, .cy=360.5100};

static apriltag_family_t *lf = nullptr;
static apriltag_detector_t *td = nullptr;
static lightanchor_detector_t *ld = nullptr;

int init()
{
    lf = lightanchor_family_create();
    if (lf == nullptr)
        return -1;

    td = apriltag_detector_create();
    if (td == nullptr)
        return -1;

    apriltag_detector_add_family(td, lf);

    ld = lightanchor_detector_create();
    if (ld == nullptr)
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

int add_code(char code)
{
    return lightanchor_detector_add_code(ld, code);
}

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

int set_quad_decimate(float quad_decimate)
{
    td->quad_decimate = quad_decimate;
    return 0;
}

int save_grayscale(uintptr_t pixelsptr, uintptr_t grayptr, int cols, int rows)
{
    uint8_t *pixels = reinterpret_cast<uint8_t*>(pixelsptr);
    uint8_t *gray = reinterpret_cast<uint8_t*>(grayptr);

    int len = cols * rows * 4;
    for (int i = 0, j = 0; i < len; i+=4, j++)
    {
        gray[j] = pixels[i];
    }
    return 0;
}

int detect_tags(uintptr_t grayptr, int cols, int rows)
{
    uint8_t *gray = reinterpret_cast<uint8_t*>(grayptr);
    image_u8_t im = {
        .width = cols,
        .height = rows,
        .stride = cols,
        .buf = gray
    };

    // // EM_ASM({console.time("quad detection")});
    // zarray_t *quads = detect_quads(td, &im);
    // // EM_ASM({console.timeEnd("quad detection")});

    // // EM_ASM({console.time("tag tracking")});
    // zarray_t *lightanchors = decode_tags(td, ld, quads, &im);
    // // EM_ASM({console.timeEnd("tag tracking")});

    zarray_t *lightanchors = lightanchor_detector_detect(td, ld, &im);

    int sz = zarray_size(lightanchors);

    EM_ASM_({
        var scope = ('function' === typeof importScripts) ? self : window;
        scope.tags = [];
    });

    for (int i = 0; i < sz; i++)
    {
        lightanchor_t *la;
        zarray_get(lightanchors, i, &la);

        EM_ASM_({
            var scope = ('function' === typeof importScripts) ? self : window;

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

            scope.tags.push(tag);
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

        EM_ASM_({
            var scope = ('function' === typeof importScripts) ? self : window;

            var $a = arguments;
            var i = 0;

            const H = [];
            H[0] = $a[i++];
            H[1] = $a[i++];
            H[2] = $a[i++];
            H[3] = $a[i++];
            H[4] = $a[i++];
            H[5] = $a[i++];
            H[6] = $a[i++];
            H[7] = $a[i++];
            H[8] = $a[i++];

            scope.tags[scope.tags.length-1].H = H;
        },
            matd_get(la->H,0,0),
            matd_get(la->H,0,1),
            matd_get(la->H,0,2),
            matd_get(la->H,1,0),
            matd_get(la->H,1,1),
            matd_get(la->H,1,2),
            matd_get(la->H,2,0),
            matd_get(la->H,2,1),
            matd_get(la->H,2,2)
        );

        apriltag_detection_t det;
        memcpy(det.c, la->c, sizeof(det.c));
        memcpy(det.p, la->p, sizeof(det.p));
        det.H = la->H;

        pose_info.det = &det;

        apriltag_pose_t pose;
        // EM_ASM({console.time("pose estimation")});
        double error = estimate_tag_pose(&pose_info, &pose);
        // EM_ASM({console.time("pose estimation")});

        EM_ASM_({
            var scope = ('function' === typeof importScripts) ? self : window;

            var $a = arguments;
            var i = 0;

            const pose = {};

            pose["error"] = $a[i++];

            const R = [];
            R[0] = $a[i++];
            R[1] = $a[i++];
            R[2] = $a[i++];
            R[3] = $a[i++];
            R[4] = $a[i++];
            R[5] = $a[i++];
            R[6] = $a[i++];
            R[7] = $a[i++];
            R[8] = $a[i++];
            pose["R"] = R;

            const t = [];
            t[0] = $a[i++];
            t[1] = $a[i++];
            t[2] = $a[i++];
            pose["t"] = t;

            scope.tags[scope.tags.length-1].pose = pose;
        },
            error,
            matd_get(pose.R,0,0),
            matd_get(pose.R,1,0),
            matd_get(pose.R,2,0),
            matd_get(pose.R,0,1),
            matd_get(pose.R,1,1),
            matd_get(pose.R,2,1),
            matd_get(pose.R,0,2),
            matd_get(pose.R,1,2),
            matd_get(pose.R,2,2),
            matd_get(pose.t,0,0),
            matd_get(pose.t,1,0),
            matd_get(pose.t,2,0)
        );
    }

    lightanchors_destroy(lightanchors);

    return sz;
}

EMSCRIPTEN_BINDINGS(flash_bindings) {
    function("init", &init);
    function("add_code", &add_code);
    function("set_detector_options", &set_detector_options);
    function("set_quad_decimate", &set_quad_decimate);
    function("save_grayscale", &save_grayscale, allow_raw_pointers());
    function("detect_tags", &detect_tags, allow_raw_pointers());
}
