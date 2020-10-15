
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
#include "lightanchor_detector.h"

static inline void homography_project(const matd_t *H, double x, double y, double *ox, double *oy)
{
    double xx = MATD_EL(H, 0, 0)*x + MATD_EL(H, 0, 1)*y + MATD_EL(H, 0, 2);
    double yy = MATD_EL(H, 1, 0)*x + MATD_EL(H, 1, 1)*y + MATD_EL(H, 1, 2);
    double zz = MATD_EL(H, 2, 0)*x + MATD_EL(H, 2, 1)*y + MATD_EL(H, 2, 2);

    *ox = xx / zz;
    *oy = yy / zz;
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

    // timeprofile_clear(td->tp);
    // timeprofile_stamp(td->tp, "init");

    ///////////////////////////////////////////////////////////
    // Step 1. Detect quads according to requested image decimation
    // and blurring parameters.
    image_u8_t *quad_im = im_orig;
    if (td->quad_decimate > 1)
    {
        quad_im = image_u8_decimate(im_orig, td->quad_decimate);

        // timeprofile_stamp(td->tp, "decimate");
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

    // timeprofile_stamp(td->tp, "blur/sharp");

    if (td->debug)
        image_u8_write_pnm(quad_im, "debug_preprocess.pnm");

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

static lightanchor_t *create_lightanchor(struct quad *quad)
{
    lightanchor_t *lightanchor = calloc(1, sizeof(lightanchor_t));
    lightanchor->p[0][0] = quad->p[0][0];
    lightanchor->p[0][1] = quad->p[0][1];
    lightanchor->p[1][0] = quad->p[1][0];
    lightanchor->p[1][1] = quad->p[1][1];
    lightanchor->p[2][0] = quad->p[2][0];
    lightanchor->p[2][1] = quad->p[2][1];
    lightanchor->p[3][0] = quad->p[3][0];
    lightanchor->p[3][1] = quad->p[3][1];
    if (quad->H) {
        lightanchor->H = matd_copy(quad->H);
        homography_project(lightanchor->H, 0, 0, &lightanchor->c[0], &lightanchor->c[1]);
        if (g2d_distance(lightanchor->c, lightanchor->p[0]) < 5 ||
            g2d_distance(lightanchor->c, lightanchor->p[1]) < 5 ||
            g2d_distance(lightanchor->c, lightanchor->p[2]) < 5 ||
            g2d_distance(lightanchor->c, lightanchor->p[3]) < 5)
            {
                goto invalid;
            }
        else {
            return lightanchor;
        }
    }

invalid:
        return NULL;
}

/** @copydoc decode_tags */
zarray_t *decode_tags(apriltag_detector_t *td, zarray_t *quads, image_u8_t *quad_im)
{
    zarray_t *lightanchors = zarray_create(sizeof(lightanchor_t));

    for (int i = 0; i < zarray_size(quads); i++)
    {
        struct quad *quad;
        zarray_get_volatile(quads, i, &quad);

        lightanchor_t *lightanchor = create_lightanchor(quad);
        if (lightanchor != NULL)
            zarray_add(lightanchors, lightanchor);
    }

    return lightanchors;
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

/** @copydoc lightanchor_destroy */
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
