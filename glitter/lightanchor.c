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
    if (quad->H == NULL)
        return NULL;

    lightanchor_t *la = calloc(1, sizeof(lightanchor_t));
    la->p[0][0] = quad->p[0][0];
    la->p[0][1] = quad->p[0][1];
    la->p[1][0] = quad->p[1][0];
    la->p[1][1] = quad->p[1][1];
    la->p[2][0] = quad->p[2][0];
    la->p[2][1] = quad->p[2][1];
    la->p[3][0] = quad->p[3][0];
    la->p[3][1] = quad->p[3][1];

    la->area = 0;

    // get area of triangle formed by points 0, 1, 2, 0
    double length[3], p;
    for (int i = 0; i < 3; i++) {
        int idxa = i; // 0, 1, 2,
        int idxb = (i+1) % 3; // 1, 2, 0
        length[i] = sqrt(sq(quad->p[idxb][0] - quad->p[idxa][0]) +
                            sq(quad->p[idxb][1] - quad->p[idxa][1]));
    }
    p = (length[0] + length[1] + length[2]) / 2;

    la->area += sqrt(p*(p-length[0])*(p-length[1])*(p-length[2]));

    // get area of triangle formed by points 2, 3, 0, 2
    for (int i = 0; i < 3; i++) {
        int idxs[] = { 2, 3, 0, 2 };
        int idxa = idxs[i];
        int idxb = idxs[i+1];
        length[i] = sqrt(sq(quad->p[idxb][0] - quad->p[idxa][0]) +
                            sq(quad->p[idxb][1] - quad->p[idxa][1]));
    }
    p = (length[0] + length[1] + length[2]) / 2;

    la->area += sqrt(p*(p-length[0])*(p-length[1])*(p-length[2]));

    la->H = matd_copy(quad->H);
    homography_project(la->H, 0, 0, &la->c[0], &la->c[1]);
    return la;
}

/** @copydoc lightanchor_copy */
lightanchor_t *lightanchor_copy(lightanchor_t *old)
{
    lightanchor_t *new = malloc(sizeof(lightanchor_t));
    memcpy(new, old, sizeof(lightanchor_t));
    if (old->H)
        new->H = matd_copy(old->H);
    return new;
}

void lightanchor_destroy(lightanchor_t *la) {
    if (la == NULL)
        return;

    if (la->H)
        matd_destroy(la->H);
    free(la);
}

void lightanchor_update(lightanchor_t *src, lightanchor_t *dest) {
    dest->valid = src->valid;
    dest->frames = src->frames;
    dest->match_code = src->match_code;
    dest->code = src->code;
    dest->next_code = src->next_code;

    qb_copy(&dest->brightnesses, &src->brightnesses);
}

static void lightanchor_stats(lightanchor_t *la, double max[], double min[]) {
    max[0] = 0;
    max[1] = 0;
    min[0] = MAX_DIST;
    min[1] = MAX_DIST;
    for (int i = 0; i < 4; i++)
    {
        if (la->p[i][0] > max[0])
            max[0] = la->p[i][0];

        if (la->p[i][0] < min[0])
            min[0] = la->p[i][0];

        if (la->p[i][1] > max[1])
            max[1] = la->p[i][1];

        if (la->p[i][1] < min[1])
            min[1] = la->p[i][1];
    }
}

uint8_t extract_brightness(lightanchor_t *la, image_u8_t *im) {
    int avg = 0, n = 0;

    double max[2], min[2];
    lightanchor_stats(la, max, min);

    int maxi[2], mini[2];
    maxi[0] = (int)ceil(max[0]-0.5);
    mini[0] = (int)ceil(min[0]-0.5);
    maxi[1] = (int)ceil(max[1]-0.5);
    mini[1] = (int)ceil(min[1]-0.5);

    zarray_t *quad_poly = g2d_polygon_create_data(la->p, 4);

    double p[2];
    for (int ix = mini[0]; ix <= maxi[0]; ix++) {
        for (int iy = mini[1]; iy <= maxi[1]; iy++) {
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

/** @copydoc lightanchors_destroy */
int lightanchors_destroy(zarray_t *lightanchors)
{
    int i = 0;
    for (; i < zarray_size(lightanchors); i++)
    {
        lightanchor_t *lightanchor;
        zarray_get(lightanchors, i, &lightanchor);
        lightanchor_destroy(lightanchor);
    }
    zarray_destroy(lightanchors);
    return i;
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
