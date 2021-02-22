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

static void lightanchor_stats(lightanchor_t *la, double max[], double min[]) {
    max[0] = 0;
    max[1] = 0;
    min[0] = MAX_DIST;
    min[1] = MAX_DIST;
    for (int i = 0; i < 4; i++)
    {
        if (la->p[i][0] > max[0]) {
            max[0] = la->p[i][0];
        }
        if (la->p[i][0] < min[0]) {
            min[0] = la->p[i][0];
        }
        if (la->p[i][1] > max[1]) {
            max[1] = la->p[i][1];
        }
        if (la->p[i][1] < min[1]) {
            min[1] = la->p[i][1];
        }
    }
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

    double max[2], min[2];
    lightanchor_stats(l, max, min);

    int maxi[2], mini[2];
    maxi[0] = (int)ceil(max[0]-0.5);
    mini[0] = (int)ceil(min[0]-0.5);
    maxi[1] = (int)ceil(max[1]-0.5);
    mini[1] = (int)ceil(min[1]-0.5);

    zarray_t *quad_poly = g2d_polygon_create_data(l->p, 4);

    double p[2];
    for (int ix = mini[0]; ix <= maxi[0]; ix+=2) {
        for (int iy = mini[1]; iy <= maxi[1]; iy+=2) {
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
