/** @file lightanchor_detector.c
 *  @brief Implementation of the lightanchor detector library
 *  @see lightanchor_detector.h for documentation
 *
 * Copyright (C) Wiselab CMU.
 * @date July, 2020
 *
 */
#include <math.h>
#include "apriltag.h"
#include "common/zarray.h"
#include "common/homography.h"
#include "common/g2d.h"
#include "common/math_util.h"
#include "common/matd.h"
#include "lightanchor.h"
#include "lightanchor_detector.h"
#include "bit_match.h"
#include "queue_buf.h"

#define MAX_DIST            1000000

lightanchor_detector_t *lightanchor_detector_create()
{
    lightanchor_detector_t *ld = (lightanchor_detector_t *)calloc(1, sizeof(lightanchor_detector_t));

    ld->candidates = zarray_create(sizeof(lightanchor_t));
    ld->detections = zarray_create(sizeof(lightanchor_t));

    ld->codes = ll_create(16);

    return ld;
}

int lightanchor_detector_add_code(lightanchor_detector_t *ld, char code) {
    uint16_t doubled_code = double_bits(code);
    ll_add(ld->codes, doubled_code);
    return 0;
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
    if (td->wp == NULL || td->nthreads != workerpool_get_nthreads(td->wp))
    {
        workerpool_destroy(td->wp);
        td->wp = workerpool_create(td->nthreads);
    }

    ///////////////////////////////////////////////////////////
    // Step 1. Detect quads according to requested image decimation
    // and blurring parameters.
    image_u8_t *quad_im = image_u8_copy(im_orig);

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

    if (quad_im != im_orig)
        image_u8_destroy(quad_im);

    return quads;
}

static void update_candidates(lightanchor_detector_t *ld, zarray_t *new_tags, image_u8_t *im)
{
    assert(new_tags != NULL);

    zarray_clear(ld->detections);

    const int im_w = im->width, im_h = im->height;
    const double thres_dist = (double)imax(im_w, im_h) / 4;

    if (zarray_size(ld->candidates) == 0)
    {
        for (int i = 0; i < zarray_size(new_tags); i++)
        {
            lightanchor_t *candidate;
            zarray_get_volatile(new_tags, i, &candidate);
            zarray_add(ld->candidates, candidate);
        }
    }
    else {
        for (int i = 0; i < zarray_size(new_tags); i++)
        {
            lightanchor_t *new_tag, *match_tag = NULL;
            zarray_get_volatile(new_tags, i, &new_tag);

            double min_dist = MAX_DIST;
            // search for closest tag
            for (int j = 0; j < zarray_size(ld->candidates); j++)
            {
                lightanchor_t *old_tag;
                zarray_get_volatile(ld->candidates, j, &old_tag);

                double dist = ( g2d_distance(old_tag->p[0], new_tag->p[0]) +
                                g2d_distance(old_tag->p[1], new_tag->p[1]) +
                                g2d_distance(old_tag->p[2], new_tag->p[2]) +
                                g2d_distance(old_tag->p[3], new_tag->p[3]) ) / 4;
                if (dist < min_dist)
                {
                    // reject tags with dissimilar shape
                    double dist_center_new = (g2d_distance(new_tag->p[0], new_tag->c) +
                                            g2d_distance(new_tag->p[1], new_tag->c) +
                                            g2d_distance(new_tag->p[2], new_tag->c) +
                                            g2d_distance(new_tag->p[3], new_tag->c)) / 4;
                    double dist_center_old = (g2d_distance(old_tag->p[0], old_tag->c) +
                                            g2d_distance(old_tag->p[1], old_tag->c) +
                                            g2d_distance(old_tag->p[2], old_tag->c) +
                                            g2d_distance(old_tag->p[3], old_tag->c)) / 4;
                    if (fabs(dist_center_new - dist_center_old) < 50)
                    {
                        min_dist = dist;
                        match_tag = old_tag;
                    }
                }
            }

            if (match_tag != NULL && min_dist < thres_dist)
            {
                // candidate_prev ==> candidate_curr
                lightanchor_t *candidate_prev = match_tag, *candidate_curr = new_tag;

                candidate_curr->valid = candidate_prev->valid;
                candidate_curr->code = candidate_prev->code;
                candidate_curr->match_code = candidate_prev->match_code;
                candidate_curr->next_code = candidate_prev->next_code;

                qb_copy(&candidate_curr->brightnesses, &candidate_prev->brightnesses);
                candidate_curr->brightness = get_brightness(candidate_curr, im);
                qb_add(&candidate_curr->brightnesses, candidate_curr->brightness);

                uint8_t max, min, thres;
                qb_stats(&candidate_curr->brightnesses, &max, &min, &thres);
                if ((max - min) > ld->range_thres)
                {
                    candidate_curr->code = (candidate_curr->code << 1) | (candidate_curr->brightness > thres);

                    // for (int i = 0; i < BUF_SIZE; i++)
                    // {
                    //     printf("%u ", candidate_curr->brightnesses.buf[i]);
                    // }
                    // printf("| %u, %u, %u\n", max, min, thres);

                    if (qb_full(&candidate_curr->brightnesses) && (match(ld, candidate_curr)))
                    {
                        zarray_add(ld->detections, candidate_curr);
                    }
                }
            }
        }

        lightanchors_destroy(ld->candidates);
        ld->candidates = new_tags;
    }
}

/** @copydoc decode_tags */
zarray_t *decode_tags(lightanchor_detector_t *ld, zarray_t *quads, image_u8_t *im)
{
    zarray_t *new_tags = zarray_create(sizeof(lightanchor_t));

    for (int i = 0; i < zarray_size(quads); i++)
    {
        struct quad *quad;
        zarray_get_volatile(quads, i, &quad);

        if (quad_update_homographies(quad))
            continue;

        lightanchor_t *lightanchor;
        if ((lightanchor = lightanchor_create(quad)) != NULL)
            zarray_add(new_tags, lightanchor);
    }
    quads_destroy(quads);

    update_candidates(ld, new_tags, im);
    return ld->detections;
}
