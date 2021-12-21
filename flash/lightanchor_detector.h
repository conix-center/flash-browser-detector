/** @file lightanchor_detector.h
 *  @brief Definitions for the lightanchor detector library
 *
 *  Use apriltag library to implement a lightanchors detector
 *  More details: (to be available)
 *
 * Copyright (C) Wiselab CMU.
 * @author Edward Lu (elu2@andrew.cmu.edu)
 * @date July, 2020
 */
#ifndef _LIGHTANCHORS_DETECT_H_
#define _LIGHTANCHORS_DETECT_H_

#include "apriltag.h"
#include "common/zarray.h"

typedef struct lightanchor_detector
{
    // min amplitude threshold for filtering out non-blinking quads
    int range_thres;

    // number of frames to live for interpolation
    int ttl_frames;

    // threshold for shape difference
    double thres_dist_shape;

    // threshold for shape difference for ttl quads
    double thres_dist_shape_ttl;

    // threshold for center difference between frames
    double thres_dist_center;

    // list of codes to detect
    zarray_t *codes;

    // list of possible lightanchor candidates (may not all be real detections)
    zarray_t *candidates;
} lightanchor_detector_t;

/**
 * Create a lightanchor apriltag family.
 *
 * @return apriltag_family_t represeting a lightanchor
 */
apriltag_family_t *lightanchor_family_create();

/**
 * Create a lightanchor detector.
 *
 * @return lightanchor detector
 */
lightanchor_detector_t *lightanchor_detector_create();

/**
 * Decodes a list of incoming quads to see if any of the quads are lightanchors.
 *
 * @return zarray_t of detected lightanchors.
 */
void lightanchor_detector_destroy(lightanchor_detector_t *ld);

/**
 * Add a code (sequence of bits) to detect.
 *
 * @return 0 on success, negative on failure
 */
int lightanchor_detector_add_code(lightanchor_detector_t *ld, char code);

/**
 * Decodes a list of incoming quads to see if any of the quads are lightanchors.
 *
 * @return zarray_t of detected lightanchors.
 */
zarray_t *decode_quads(apriltag_detector_t *td, lightanchor_detector_t *ld, zarray_t *quads, image_u8_t *im);

/**
 * Detects lightacnhors in a frame of video (im).
 *
 * @return zarray_t of detected lightanchors.
 */
zarray_t *lightanchor_detector_detect(apriltag_detector_t *td, lightanchor_detector_t *ld, image_u8_t *im);

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
