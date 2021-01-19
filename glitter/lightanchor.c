#include <math.h>
#include "apriltag.h"
#include "common/zarray.h"
#include "common/homography.h"
#include "common/g2d.h"
#include "common/math_util.h"
#include "lightanchor.h"
#include "queue_buf.h"
#include <emscripten/emscripten.h>

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

    l->next_code = 0;

    if (quad->H) {
        l->H = matd_copy(quad->H);
        homography_project(l->H, 0, 0, &l->c[0], &l->c[1]);
        // if the center is within 50px of any of the quad points ==> too small ==> invalid
        if (g2d_distance(l->c, l->p[0]) > 50 &&
            g2d_distance(l->c, l->p[1]) > 50 &&
            g2d_distance(l->c, l->p[2]) > 50 &&
            g2d_distance(l->c, l->p[3]) > 50) {
            return l;
        }
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

    const double px0d = l->p[0][0], py0d = l->p[0][1], px1d = l->p[1][0], py1d = l->p[1][1];
    const double px2d = l->p[2][0], py2d = l->p[2][1], px3d = l->p[3][0], py3d = l->p[3][1];
    const int px0 = ceil(px0d - 0.5), py0 = ceil(py0d - 0.5), px1 = ceil(px1d - 0.5), py1 = ceil(py1d - 0.5);
    const int px2 = ceil(px2d - 0.5), py2 = ceil(py2d - 0.5), px3 = ceil(px3d - 0.5), py3 = ceil(py3d - 0.5);

    int max_x = 0;
    max_x = imax(px0, max_x);
    max_x = imax(px1, max_x);
    max_x = imax(px2, max_x);
    max_x = imax(px3, max_x);

    int min_x = im->width;
    min_x = imin(px0, min_x);
    min_x = imin(px1, min_x);
    min_x = imin(px2, min_x);
    min_x = imin(px3, min_x);

    int max_y = 0;
    max_y = imax(py0, max_y);
    max_y = imax(py1, max_y);
    max_y = imax(py2, max_y);
    max_y = imax(py3, max_y);

    int min_y = im->height;
    min_y = imin(py0, min_y);
    min_y = imin(py1, min_y);
    min_y = imin(py2, min_y);
    min_y = imin(py3, min_y);

    zarray_t *quad_poly = g2d_polygon_create_data(l->p, 4);

    double p[2] = {-1,-1};
    for (int ix = min_x; ix <= max_x; ix+=4) {
        for (int iy = min_y; iy <= max_y; iy+=4) {
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
