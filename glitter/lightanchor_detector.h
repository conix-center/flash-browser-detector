 /** @file lightanchor_detector.h
 *  @brief Definitions for the lightanchor detector library
 *
 *  Use apriltag library to implement a lightanchors detector
 *  More details: (to be available)
 *
 * Copyright (C) Wiselab CMU.
 * @date July, 2020
 */

#ifndef _LIGHTANCHORS_DETECT_
#define _LIGHTANCHORS_DETECT_
#include "apriltag.h"
#include "common/zarray.h"

/* declare functions that we need as extern */
extern zarray_t *apriltag_quad_thresh(apriltag_detector_t *td, image_u8_t *im);
extern int quad_update_homographies(struct quad *quad);
extern struct quad *quad_copy(struct quad *quad);
extern double g2d_distance(const double a[2], const double b[2]);

typedef struct lightanchor_detector lightanchor_detector_t;
struct lightanchor_detector
{
    uint8_t blink_freq;
    zarray_t *candidates;
    zarray_t *detections;
    uint16_t codes[8];
};

typedef struct lightanchor lightanchor_t;
struct lightanchor
{
    uint8_t valid;
    uint8_t brightness;
    struct ll *brightnesses;
    uint16_t code;
    uint16_t next_code;
    uint16_t counter;
    matd_t *H;
    double c[2];
    double p[4][2];
};

lightanchor_detector_t *lightanchor_detector_create(char code);
lightanchor_t *lightanchor_create(struct quad *quad, image_u8_t *im);
void lightanchor_detector_destroy(lightanchor_detector_t *ld);

zarray_t *decode_tags(lightanchor_detector_t *ld, zarray_t *quads, image_u8_t *im);
lightanchor_t *lightanchor_copy(lightanchor_t *lightanchor);
int lightanchors_destroy(zarray_t *lightanchors);

/**
 * Use apriltag library to detect quads from an image and
 * return an array of these (struct quad).
 *
 * Caller *must free* returned array with quads_destroy()
 *
 * This is step 1 of the apriltag detector:
 *  https://github.com/AprilRobotics/apriltag/blob/master/apriltag.c
 *
 * @param *td an initialized apriltag detector
 * @param *im_orig grayscale image to perform the detection on
 *
 * @return z_array of struct quad
 */
zarray_t *detect_quads(apriltag_detector_t *td, image_u8_t *im_orig);


/**
 * Free an array of quads
 *
 * @param *quads z_array of quads
 *
 * @return number of quads freed
 */
int quads_destroy(zarray_t *quads);

#endif
