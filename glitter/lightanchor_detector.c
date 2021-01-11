
/** @file lightanchor_detector.c
 *  @brief Implementation of the lightanchor detector library
 *  @see lightanchor_detector.h for documentation
 *
 * Copyright (C) Wiselab CMU.
 * @date July, 2020
 *
 * @todo Actually implement the LED detector
 */

#include <math.h>
#include "apriltag.h"
#include "common/zarray.h"
#include "common/homography.h"
#include "common/g2d.h"
#include "common/math_util.h"
#include "lightanchor.h"
#include "lightanchor_detector.h"
#include "bit_match.h"
#include "queue_buf.h"

#define RANGE_THRES         85

lightanchor_detector_t *lightanchor_detector_create(char code)
{
    lightanchor_detector_t *ld = (lightanchor_detector_t*)calloc(1, sizeof(lightanchor_detector_t));

    ld->candidates = zarray_create(sizeof(lightanchor_t));
    ld->detections = zarray_create(sizeof(lightanchor_t));

    ld->code = double_bits(code);

    return ld;
}

void lightanchor_detector_destroy(lightanchor_detector_t *ld)
{
    lightanchors_destroy(ld->candidates);
    lightanchors_destroy(ld->detections);
    free(ld);
}

/** @copydoc detect_quads */
zarray_t *detect_quads(apriltag_detector_t *td, image_u8_t *im_orig)
{
    if (zarray_size(td->tag_families) == 0)
    {
        zarray_t *s = zarray_create(sizeof(apriltag_detection_t *));
        printf("apriltag.c: No tag families enabled.");
        return s;
    }

    if (td->wp == NULL || td->nthreads != workerpool_get_nthreads(td->wp))
    {
        workerpool_destroy(td->wp);
        td->wp = workerpool_create(td->nthreads);
    }

    ///////////////////////////////////////////////////////////
    // Step 1. Detect quads according to requested image decimation
    // and blurring parameters.
    image_u8_t *quad_im = im_orig;
    if (td->quad_decimate > 1)
    {
        quad_im = image_u8_decimate(im_orig, td->quad_decimate);
    }

    if (td->quad_sigma != 0)
    {
        // compute a reasonable kernel width by figuring that the
        // kernel should go out 2 std devs.
        //
        // max sigma          ksz
        // 0.499              1  (disabled)
        // 0.999              3
        // 1.499              5
        // 1.999              7

        float sigma = fabsf((float)td->quad_sigma);

        int ksz = 4 * sigma; // 2 std devs in each direction
        if ((ksz & 1) == 0)
            ksz++;

        if (ksz > 1)
        {

            if (td->quad_sigma > 0)
            {
                // Apply a blur
                image_u8_gaussian_blur(quad_im, sigma, ksz);
            }
            else
            {
                // SHARPEN the image by subtracting the low frequency components.
                image_u8_t *orig = image_u8_copy(quad_im);
                image_u8_gaussian_blur(quad_im, sigma, ksz);

                for (int y = 0; y < orig->height; y++)
                {
                    for (int x = 0; x < orig->width; x++)
                    {
                        int vorig = orig->buf[y * orig->stride + x];
                        int vblur = quad_im->buf[y * quad_im->stride + x];

                        int v = 2 * vorig - vblur;
                        if (v < 0)
                            v = 0;
                        if (v > 255)
                            v = 255;

                        quad_im->buf[y * quad_im->stride + x] = (uint8_t)v;
                    }
                }
                image_u8_destroy(orig);
            }
        }
    }

    zarray_t *quads = apriltag_quad_thresh(td, quad_im);

    // adjust centers of pixels so that they correspond to the
    // original full-resolution image.
    if (td->quad_decimate > 1)
    {
        for (int i = 0; i < zarray_size(quads); i++)
        {
            struct quad *q;
            zarray_get_volatile(quads, i, &q);

            for (int j = 0; j < 4; j++)
            {
                if (td->quad_decimate == 1.5)
                {
                    q->p[j][0] *= td->quad_decimate;
                    q->p[j][1] *= td->quad_decimate;
                }
                else
                {
                    q->p[j][0] = (q->p[j][0] - 0.5) * td->quad_decimate + 0.5;
                    q->p[j][1] = (q->p[j][1] - 0.5) * td->quad_decimate + 0.5;
                }
            }
        }
    }

    zarray_t *quads_valid = zarray_create(sizeof(struct quad));

    double det;
    for (int i = 0; i < zarray_size(quads); i++)
    {
        struct quad *quad;
        zarray_get_volatile(quads, i, &quad);

        // find homographies for each detected quad
        quad_update_homographies(quad);

        det = MATD_EL(quad->H,0,0) * MATD_EL(quad->H,1,1) - MATD_EL(quad->H,1,0) * MATD_EL(quad->H,0,1);
        if (det < 0.0) continue;

        zarray_add(quads_valid, quad_copy(quad));
    }

    quads_destroy(quads);

    if (quad_im != im_orig)
        image_u8_destroy(quad_im);

    return quads_valid;
}

static void update_candidates(lightanchor_detector_t *ld, zarray_t *local_tags, image_u8_t *im) {
    assert(local_tags != NULL);

    zarray_clear(ld->detections);

    const int im_w = im->width, im_h = im->height;
    const int64_t max_dist = sqrtf(im_w*im_w+im_h*im_h);
    const double thres_dist = (double)imax(im_w, im_h) / 10;

    if (zarray_size(ld->candidates) == 0) {
        for (int i = 0; i < zarray_size(local_tags); i++)
        {
            lightanchor_t *candidate;
            zarray_get_volatile(local_tags, i, &candidate);
            zarray_add(ld->candidates, lightanchor_copy(candidate));
        }
    }
    else {
        zarray_t *valid = zarray_create(sizeof(lightanchor_t));

        for (int i = 0; i < zarray_size(ld->candidates); i++)
        {
            lightanchor_t *global_tag;
            zarray_get_volatile(ld->candidates, i, &global_tag);

            int match_idx = 0;
            double min_dist = max_dist;
            // search for closest local tag
            for (int j = 0; j < zarray_size(local_tags); j++)
            {
                lightanchor_t *local_tag;
                zarray_get_volatile(local_tags, j, &local_tag);

                double dist;
                if ((dist = g2d_distance(global_tag->c, local_tag->c)) < min_dist) {
                    min_dist = dist;
                    match_idx = j;
                }
            }

            if (min_dist < thres_dist) {
                // candidate_prev ==> candidate_curr
                lightanchor_t *candidate_prev = global_tag, *candidate_curr;
                zarray_get_volatile(local_tags, match_idx, &candidate_curr);

                candidate_curr->valid = candidate_prev->valid;
                candidate_curr->code = candidate_prev->code;
                candidate_curr->next_code = candidate_prev->next_code;

                memcpy(&candidate_curr->brightnesses, &candidate_prev->brightnesses, sizeof(struct queue_buf));
                candidate_curr->brightness = get_brightness(candidate_curr, im);

                uint8_t max, min;
                qb_add(&candidate_curr->brightnesses, candidate_curr->brightness);
                qb_stats(&candidate_curr->brightnesses, &max, &min);
                uint8_t thres = (max + min) / 2;
                if ((max - min) > RANGE_THRES) {
                    candidate_curr->code = (candidate_curr->code << 1) | (candidate_curr->brightness > thres);

                    // for (int i = 0; i < BUF_SIZE; i++) {
                    //     printf("%u ", candidate_curr->brightnesses.buf[i]);
                    // }
                    // printf("| %u, %u, %u\n", max, min, thres);

                    if (match(ld, candidate_curr)) {
                        zarray_add(ld->detections, candidate_curr);
                    }
                    zarray_add(valid, candidate_curr);
                }
            }
        }
        // lightanchors_destroy(ld->candidates);
        ld->candidates = valid;
    }
}

/** @copydoc decode_tags */
zarray_t *decode_tags(lightanchor_detector_t *ld, zarray_t *quads, image_u8_t *im)
{
    zarray_t *local_tags = zarray_create(sizeof(lightanchor_t));

    for (int i = 0; i < zarray_size(quads); i++)
    {
        struct quad *quad;
        zarray_get_volatile(quads, i, &quad);

        lightanchor_t *lightanchor;
        if ((lightanchor = lightanchor_create(quad)) != NULL)
            zarray_add(local_tags, lightanchor_copy(lightanchor));
    }

    update_candidates(ld, local_tags, im);

    lightanchors_destroy(local_tags);

    return ld->detections;
}
