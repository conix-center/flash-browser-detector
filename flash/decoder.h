#ifndef _DECODER_H_
#define _DECODER_H_

#include "apriltag.h"
#include "common/zarray.h"
#include "lightanchor.h"
#include "lightanchor_detector.h"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)   \
    (byte & 0x80 ? '1' : '0'), \
    (byte & 0x40 ? '1' : '0'), \
    (byte & 0x20 ? '1' : '0'), \
    (byte & 0x10 ? '1' : '0'), \
    (byte & 0x08 ? '1' : '0'), \
    (byte & 0x04 ? '1' : '0'), \
    (byte & 0x02 ? '1' : '0'), \
    (byte & 0x01 ? '1' : '0')

typedef struct {
    uint8_t code;
    uint16_t doubled_code;
} glitter_code_t;

// size_t hamming_dist(size_t a, size_t b);
uint16_t double_bits(uint8_t bits);
uint8_t undouble_bits(uint16_t bits);
int lightanchor_decode(lightanchor_detector_t *ld, lightanchor_t *candidate_curr);

#endif
