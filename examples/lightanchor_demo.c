/* Copyright (C) 2013-2016, The Regents of The University of Michigan.
All rights reserved.

This software was developed in the APRIL Robotics Lab under the
direction of Edwin Olson, ebolson@umich.edu. This software may be
available under alternative licensing terms; contact the address above.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the Regents of The University of Michigan.
*/

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>

#include "apriltag.h"
#include "tag36h11.h"

#include "common/getopt.h"
#include "common/image_u8.h"
#include "common/image_u8x4.h"
#include "common/pjpeg.h"
#include "common/zarray.h"

#include "lightanchor.h"
#include "lightanchor_detector.h"

#include "common/postscript_utils.h"

// Invoke:
//
// tagtest [options] input.pnm

int main(int argc, char *argv[])
{
    getopt_t *getopt = getopt_create();

    getopt_add_bool(getopt, 'h', "help", 0, "Show this help");
    getopt_add_bool(getopt, 'd', "debug", 0, "Enable debugging output (slow)");
    getopt_add_bool(getopt, 'q', "quiet", 0, "Reduce output");
    getopt_add_string(getopt, 'f', "family", "tag36h11", "Tag family to use");
    getopt_add_int(getopt, 'i', "iters", "1", "Repeat processing on input set this many times");
    getopt_add_int(getopt, 't', "threads", "1", "Use this many CPU threads");
    getopt_add_int(getopt, 'a', "hamming", "1", "Detect tags with up to this many bit errors.");
    getopt_add_double(getopt, 'x', "decimate", "2.0", "Decimate input image by this factor");
    getopt_add_double(getopt, 'b', "blur", "0.0", "Apply low-pass blur to input; negative sharpens");
    getopt_add_bool(getopt, '0', "refine-edges", 1, "Spend more time trying to align edges of tags");

    if (!getopt_parse(getopt, argc, argv, 1) || getopt_get_bool(getopt, "help"))
    {
        printf("Usage: %s [options] <input files>\n", argv[0]);
        getopt_do_usage(getopt);
        exit(0);
    }

    const zarray_t *inputs = getopt_get_extra_args(getopt);

    apriltag_family_t *tf = NULL;
    const char *famname = getopt_get_string(getopt, "family");
    if (!strcmp(famname, "tag36h11"))
    {
        tf = tag36h11_create();
    }
    else
    {
        printf("Unsupported tag family name. This example only supports \"tag36h11\".\n");
        exit(-1);
    }

    lightanchor_detector_t *ld = lightanchor_detector_create();
    lightanchor_detector_add_code(ld, 0xaf);

    apriltag_detector_t *td = apriltag_detector_create();
    apriltag_detector_add_family_bits(td, tf, getopt_get_int(getopt, "hamming"));
    td->quad_decimate = getopt_get_double(getopt, "decimate");
    td->quad_sigma = getopt_get_double(getopt, "blur");
    td->nthreads = getopt_get_int(getopt, "threads");
    td->debug = getopt_get_bool(getopt, "debug");
    td->refine_edges = getopt_get_bool(getopt, "refine-edges");

    int quiet = getopt_get_bool(getopt, "quiet");

    for (int input = 0; input < zarray_size(inputs); input++)
    {

        char *path;
        zarray_get(inputs, input, &path);
        if (!quiet)
            printf("loading %s\n", path);
        else
            printf("%20s ", path);

        image_u8_t *im = NULL;
        if (str_ends_with(path, "pnm") || str_ends_with(path, "PNM") ||
            str_ends_with(path, "pgm") || str_ends_with(path, "PGM"))
            im = image_u8_create_from_pnm(path);
        else if (str_ends_with(path, "jpg") || str_ends_with(path, "JPG"))
        {
            int err = 0;
            pjpeg_t *pjpeg = pjpeg_create_from_file(path, 0, &err);
            if (pjpeg == NULL)
            {
                printf("pjpeg error %d\n", err);
                continue;
            }

            if (1)
            {
                im = pjpeg_to_u8_baseline(pjpeg);
            }
            else
            {
                printf("illumination invariant\n");

                image_u8x3_t *imc = pjpeg_to_u8x3_baseline(pjpeg);

                im = image_u8_create(imc->width, imc->height);

                for (int y = 0; y < imc->height; y++)
                {
                    for (int x = 0; x < imc->width; x++)
                    {
                        double r = imc->buf[y * imc->stride + 3 * x + 0] / 255.0;
                        double g = imc->buf[y * imc->stride + 3 * x + 1] / 255.0;
                        double b = imc->buf[y * imc->stride + 3 * x + 2] / 255.0;

                        double alpha = 0.42;
                        double v = 0.5 + log(g) - alpha * log(b) - (1 - alpha) * log(r);
                        int iv = v * 255;
                        if (iv < 0)
                            iv = 0;
                        if (iv > 255)
                            iv = 255;

                        im->buf[y * im->stride + x] = iv;
                    }
                }
                image_u8x3_destroy(imc);
                if (td->debug)
                    image_u8_write_pnm(im, "debug_invariant.pnm");
            }

            pjpeg_destroy(pjpeg);
        }

        if (im == NULL)
        {
            printf("couldn't load %s\n", path);
            continue;
        }

        zarray_t *quads = detect_quads(td, im);

        zarray_t *lightanchors = decode_tags(td, ld, quads, im);

        if (!quiet)
            printf("Found %d lightanchors.\n", zarray_size(lightanchors));

        // output ps file
        image_u8_t *darker = image_u8_copy(im);
        image_u8_darken(darker);
        image_u8_darken(darker);

        // assume letter, which is 612x792 points.
        FILE *f = fopen("quad_output.ps", "w");
        fprintf(f, "%%!PS\n\n");
        double scale = fmin(612.0 / darker->width, 792.0 / darker->height);
        fprintf(f, "%f %f scale\n", scale, scale);
        fprintf(f, "0 %d translate\n", darker->height);
        fprintf(f, "1 -1 scale\n");
        postscript_image(f, darker);

        image_u8_destroy(darker);

        for (int i = 0; i < zarray_size(lightanchors); i++)
        {
            lightanchor_t *lightanchor;
            zarray_get(lightanchors, i, &lightanchor);

            float rgb[3];
            int bias = 100;

            for (int j = 0; j < 3; j++)
            {
                rgb[j] = bias + (random() % (255 - bias));
            }
            fprintf(f, "%f %f %f setrgbcolor\n", rgb[0] / 255.0f, rgb[1] / 255.0f, rgb[2] / 255.0f);
            fprintf(f, "%f %f moveto %f %f lineto %f %f lineto %f %f lineto %f %f lineto stroke\n",
                    lightanchor->p[0][0], lightanchor->p[0][1],
                    lightanchor->p[1][0], lightanchor->p[1][1],
                    lightanchor->p[2][0], lightanchor->p[2][1],
                    lightanchor->p[3][0], lightanchor->p[3][1],
                    lightanchor->p[0][0], lightanchor->p[0][1]);
            fprintf(f, "%f %f 1 0 360 arc stroke\n", lightanchor->c[0], lightanchor->c[1]);

            if (!quiet)
                printf("lightanchor %d [%.2f, %.2f]: (%.2f, %.2f) (%.2f, %.2f) (%.2f, %.2f) (%.2f, %.2f)\n", i, lightanchor->c[0], lightanchor->c[1],
                        lightanchor->p[0][0], lightanchor->p[0][1], lightanchor->p[1][0], lightanchor->p[1][1], lightanchor->p[2][0], lightanchor->p[2][1], lightanchor->p[3][0], lightanchor->p[3][1]);
        }

        fprintf(f, "showpage\n");
        fclose(f);

        image_u8_destroy(im);
    }

    printf("\n");

    // don't deallocate contents of inputs; those are the argv
    apriltag_detector_destroy(td);

    lightanchor_detector_destroy(ld);

    tag36h11_destroy(tf);

    getopt_destroy(getopt);

    return 0;
}
