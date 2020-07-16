
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
#include "lightanchor_detector.h"

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

    timeprofile_clear(td->tp);
    timeprofile_stamp(td->tp, "init");

    ///////////////////////////////////////////////////////////
    // Step 1. Detect quads according to requested image decimation
    // and blurring parameters.
    image_u8_t *quad_im = im_orig;
    if (td->quad_decimate > 1)
    {
        quad_im = image_u8_decimate(im_orig, td->quad_decimate);

        timeprofile_stamp(td->tp, "decimate");
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

    timeprofile_stamp(td->tp, "blur/sharp");

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

    if (quad_im != im_orig)
        image_u8_destroy(quad_im);

    return quads;
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