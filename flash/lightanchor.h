#ifndef _LIGHTANCHOR_H_
#define _LIGHTANCHOR_H_

#include "apriltag.h"
#include "common/zarray.h"
#include "queue_buf.h"

#define MAX_DIST    1000000

/* declare functions that we need as extern */
extern double value_for_pixel(image_u8_t *im, double px, double py);
extern double g2d_distance(const double a[2], const double b[2]);

typedef struct lightanchor lightanchor_t;
struct lightanchor
{
    char valid;
    uint8_t match_code;

    uint16_t code;
    uint16_t next_code;

    int frames;

    double min_dist;

    matd_t *H;

    double c[2];
    double p[4][2];

    struct queue_buf brightnesses;
};

lightanchor_t *lightanchor_create(struct quad *quad);
lightanchor_t *lightanchor_copy(lightanchor_t *lightanchor);
void lightanchor_update(lightanchor_t *src, lightanchor_t *dest);
void lightanchor_destroy(lightanchor_t *lightanchor);
int lightanchors_destroy(zarray_t *lightanchors);
uint8_t lightanchor_intensity(lightanchor_t *l, image_u8_t *im);
int quads_destroy(zarray_t *quads);

#endif
