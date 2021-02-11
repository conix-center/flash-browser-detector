#include <math.h>
#include "apriltag.h"
#include "common/zarray.h"
#include "common/homography.h"
#include "common/g2d.h"
#include "common/math_util.h"
#include "lightanchor.h"
#include "queue_buf.h"

lightanchor_t *lightanchor_create(struct quad *quad)
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

    if (quad->H) {
        l->H = matd_copy(quad->H);
        homography_project(l->H, 0, 0, &l->c[0], &l->c[1]);
        return l;
    }

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

void lightanchor_destroy(lightanchor_t *lightanchor) {
    if (!lightanchor)
        return;
    matd_destroy(lightanchor->H);
    free(lightanchor);
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

uint8_t get_brightness(lightanchor_t *l, image_u8_t *im) {
    int avg = 0, n = 0;

    const double p0xd = l->p[0][0], p0yd = l->p[0][1], p1xd = l->p[1][0], p1yd = l->p[1][1];
    const double p2xd = l->p[2][0], p2yd = l->p[2][1], p3xd = l->p[3][0], p3yd = l->p[3][1];
    const int p0x = ceil(p0xd - 0.5), p0y = ceil(p0yd - 0.5), p1x = ceil(p1xd - 0.5), p1y = ceil(p1yd - 0.5);
    const int p2x = ceil(p2xd - 0.5), p2y = ceil(p2yd - 0.5), p3x = ceil(p3xd - 0.5), p3y = ceil(p3yd - 0.5);

    int max_x = 0;
    max_x = imax(p0x, max_x);
    max_x = imax(p1x, max_x);
    max_x = imax(p2x, max_x);
    max_x = imax(p3x, max_x);

    int min_x = im->width;
    min_x = imin(p0x, min_x);
    min_x = imin(p1x, min_x);
    min_x = imin(p2x, min_x);
    min_x = imin(p3x, min_x);

    int max_y = 0;
    max_y = imax(p0y, max_y);
    max_y = imax(p1y, max_y);
    max_y = imax(p2y, max_y);
    max_y = imax(p3y, max_y);

    int min_y = im->height;
    min_y = imin(p0y, min_y);
    min_y = imin(p1y, min_y);
    min_y = imin(p2y, min_y);
    min_y = imin(p3y, min_y);

    zarray_t *quad_poly = g2d_polygon_create_data(l->p, 4);

    double p[2];
    for (int ix = min_x; ix <= max_x; ix+=2) {
        for (int iy = min_y; iy <= max_y; iy+=2) {
            p[0] = (double)ix;
            p[1] = (double)iy;
            if (g2d_polygon_contains_point(quad_poly, p)) {
                avg += value_for_pixel(im, ix, iy);
                n++;
            }
        }
    }

    uint8_t res = 0;
    if (n > 0) {
        res = (uint8_t)(avg / n);
    }
    zarray_destroy(quad_poly);
    return res;
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
