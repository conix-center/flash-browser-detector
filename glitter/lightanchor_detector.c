
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
#include "common/g2d.h"
#include "linked_list.h"
#include "lightanchor_detector.h"
#include "bit_match.h"

static inline void homography_project(const matd_t *H, double x, double y, double *ox, double *oy)
{
    double xx = MATD_EL(H, 0, 0)*x + MATD_EL(H, 0, 1)*y + MATD_EL(H, 0, 2);
    double yy = MATD_EL(H, 1, 0)*x + MATD_EL(H, 1, 1)*y + MATD_EL(H, 1, 2);
    double zz = MATD_EL(H, 2, 0)*x + MATD_EL(H, 2, 1)*y + MATD_EL(H, 2, 2);

    *ox = xx / zz;
    *oy = yy / zz;
}

lightanchor_detector_t *lightanchor_detector_create(char code)
{
    lightanchor_detector_t *ld = (lightanchor_detector_t*) calloc(1, sizeof(lightanchor_detector_t));

    ld->blink_freq = 15; // Hz

    ld->candidates = zarray_create(sizeof(lightanchor_t));
    ld->detections = zarray_create(sizeof(lightanchor_t));

    for (int i = 0; i < 8; i++)
    {
        ld->codes[i] = double_bits(code);
        code = (code << 1) | ((code >> 7) & 0x1);
    }

#if 0
    for (int i = 0; i < 8; i++) {
        printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"\n",
                BYTE_TO_BINARY(ld->codes[i]>>8), BYTE_TO_BINARY(ld->codes[i]));
    }
#endif

    return ld;
}

void lightanchor_detector_destroy(lightanchor_detector_t *ld)
{
    zarray_destroy(ld->candidates);
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

static void update_candidates(lightanchor_detector_t *ld, zarray_t *candidate_tags, int32_t im_w, int32_t im_h) {
    assert(candidate_tags != NULL);

    zarray_clear(ld->detections);

    const int64_t max_dist = sqrtf(im_w*im_w+im_h*im_h);
    const int thres_dist = im_w / 50;

    zarray_t *valid = zarray_create(sizeof(lightanchor_t));
    if (zarray_size(ld->candidates) == 0) {
        for (int i = 0; i < zarray_size(candidate_tags); i++)
        {
            lightanchor_t *candidate;
            zarray_get_volatile(candidate_tags, i, &candidate);

            zarray_add(valid, lightanchor_copy(candidate));
        }
    }
    else {
        for (int i = 0; i < zarray_size(candidate_tags); i++)
        {
            lightanchor_t *candidate;
            zarray_get_volatile(candidate_tags, i, &candidate);

            int match_idx = 0;
            double min_dist = max_dist;
            // search for closest global tag
            for (int j = 0; j < zarray_size(ld->candidates); j++)
            {
                lightanchor_t *global_tag;
                zarray_get_volatile(ld->candidates, j, &global_tag);

                double dist;
                if ((dist = g2d_distance(candidate->c, global_tag->c)) < min_dist) {
                    min_dist = dist;
                    match_idx = j;
                }
            }

            if (min_dist < (double)thres_dist) {
                // candidate_prev ==> candidate_curr
                lightanchor_t *candidate_prev, *candidate_curr = lightanchor_copy(candidate);
                zarray_get_volatile(ld->candidates, match_idx, &candidate_prev);

                candidate_curr->valid = candidate_prev->valid;
                candidate_curr->brightnesses = candidate_prev->brightnesses;
                ll_add(candidate_curr->brightnesses, candidate_curr->brightness);
                candidate_curr->next_code = candidate_prev->next_code;
                candidate_curr->counter = candidate_prev->counter;

                if (match(ld, candidate_curr)) {
                    zarray_add(ld->detections, candidate_curr);
                }

                zarray_add(valid, candidate_curr);
            }
        }
    }

    lightanchors_destroy(ld->candidates);
    ld->candidates = valid;
}

/** @copydoc decode_tags */
zarray_t *decode_tags(lightanchor_detector_t *ld, zarray_t *quads, image_u8_t *im)
{
    zarray_t *candidate_tags = zarray_create(sizeof(lightanchor_t));

    for (int i = 0; i < zarray_size(quads); i++)
    {
        struct quad *quad;
        zarray_get_volatile(quads, i, &quad);

        lightanchor_t *lightanchor;
        if ((lightanchor = lightanchor_create(quad, im)) != NULL)
            zarray_add(candidate_tags, lightanchor_copy(lightanchor));
    }

    // int max = 0, min = 255;
    // for (int i = 0; i < im->width*im->height; i++) {
    //     if (im->buf[i] > max) max = im->buf[i];
    //     if (im->buf[i] < min) min = im->buf[i];
    // }
    // printf("%u\n", (min + max)/2);

    update_candidates(ld, candidate_tags, im->width, im->height);

    lightanchors_destroy(candidate_tags);

    return ld->detections;
}

/** @copydoc quads_destroy */
int quads_destroy(zarray_t *quads)
{
    int i = 0;
    for (i = 0; i < zarray_size(quads); i++)
    {
        struct quad *quad;
        zarray_get_volatile(quads, i, &quad);
        matd_destroy(quad->H);
        matd_destroy(quad->Hinv);
    }
    zarray_destroy(quads);
    return i;
}

lightanchor_t *lightanchor_create(struct quad *quad, image_u8_t *im)
{
    lightanchor_t *l = calloc(1, sizeof(lightanchor_t));
    l->p[0][0] = quad->p[0][0];
    l->p[0][1] = quad->p[0][1];
    l->p[1][0] = quad->p[1][0];
    l->p[1][1] = quad->p[1][1];
    l->p[2][0] = quad->p[2][0];
    l->p[2][1] = quad->p[2][1];
    l->p[3][0] = quad->p[3][0];
    l->p[3][1] = quad->p[3][1];

    l->next_code = 0;
    l->brightnesses = ll_create(16);

    if (quad->H) {
        l->H = matd_copy(quad->H);
        homography_project(l->H, 0, 0, &l->c[0], &l->c[1]);
        // if the center is within 10px of any of the quad points ==> too small ==> invalid
        if (g2d_distance(l->c, l->p[0]) < 10 ||
            g2d_distance(l->c, l->p[1]) < 10 ||
            g2d_distance(l->c, l->p[2]) < 10 ||
            g2d_distance(l->c, l->p[3]) < 10)
            {
                goto invalid;
            }
        else {
            uint32_t avg = 0;
            int cx = l->c[0], cy = l->c[1];
            avg += im->buf[cy * im->stride + cx];
            avg += im->buf[cy * im->stride + cx + 1];
            avg += im->buf[cy * im->stride + cx - 1];
            avg += im->buf[(cy+1) * im->stride + cx];
            avg += im->buf[(cy-1) * im->stride + cx];
            l->brightness = (uint8_t)(avg / 5);
            return l;
        }
    }

invalid:
    return NULL;
}

/** @copydoc lightanchor_copy */
lightanchor_t *lightanchor_copy(lightanchor_t *lightanchor)
{
    lightanchor_t *l = calloc(1, sizeof(lightanchor_t));
    memcpy(l, lightanchor, sizeof(lightanchor_t));
    if (lightanchor->H)
        l->H = matd_copy(lightanchor->H);
    return l;
}

/** @copydoc lightanchors_destroy */
int lightanchors_destroy(zarray_t *lightanchors)
{
    int i = 0;
    for (i = 0; i < zarray_size(lightanchors); i++)
    {
        lightanchor_t *lightanchor;
        zarray_get_volatile(lightanchors, i, &lightanchor);
        matd_destroy(lightanchor->H);
    }
    zarray_destroy(lightanchors);
    return i;
}
