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

uint8_t lightanchor_intensity(lightanchor_t *la, image_u8_t *im) {
    int n = 0;
    double avg = 0;

    // quad goes from [-1, 1] in x and y direction
    // find approximate average brightness of entire quad
    double px, py;
    for (double ix = -1; ix <= 1; ix+=0.1) {
        for (double iy = -1; iy <= 1; iy+=0.1) {
            homography_project(la->H, ix, iy, &px, &py);
            avg += value_for_pixel(im, px, py);
            n++;
        }
    }

    return (avg / n);
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
