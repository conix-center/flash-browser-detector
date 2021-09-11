#include "decoder.h"
#include "queue_buf.h"
#include "linked_list.h"
#include "common/math_util.h"
#include "lightanchor.h"
#include "lightanchor_detector.h"

// #define DEBUG

#define EVEN_MASK       0xaaaa
#define ODD_MASK        0x5555

static inline uint16_t cyclic_lsl(uint16_t bits, uint8_t size)
{
    return (bits << 1) | ((bits >> (size - 1)) & 0x1);
}

uint16_t double_bits(uint8_t bits)
{
    uint16_t res = 0;
    for (int i = 0; i < 8; i++)
    {
        res |= ((bits & 0x1) << 1 | (bits & 0x1)) << (2*i);
        bits >>= 1;
    }
    return res;
}

uint8_t undouble_bits(uint16_t bits)
{
    uint8_t res = 0;
    for (int i = 0; i < 8; i++)
    {
        res |= ((bits & 0x1) << i);
        bits >>= 2;
    }
    return res;
}

// size_t hamming_dist(size_t a, size_t b)
// {
//     size_t c = a ^ b;
//     unsigned int hamm = 0;
//     while (c)
//     {
//         ++hamm;
//         c &= (c-1);
//     }
//     return hamm;
// }

static int match_even_odd(uint16_t a, uint16_t b, uint8_t *match)
{
    uint16_t a_even = a & EVEN_MASK, a_odd = a & ODD_MASK;
    uint16_t b_even = b & EVEN_MASK, b_odd = b & ODD_MASK;

    if (a_even == b_even)
    {
        if (match) *match = undouble_bits(b_even >> 1);
        return 1;
    }
    else if (a_odd == b_odd)
    {
        if (match) *match = undouble_bits(b_odd);
        return 1;
    }
    else {
        if (match) *match = 0xff; // invalid
        return 0;
    }
}

int lightanchor_decode(lightanchor_detector_t *ld, lightanchor_t *candidate_curr)
{
#ifdef DEBUG
    printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN" ",
            BYTE_TO_BINARY(candidate_curr->code>>8), BYTE_TO_BINARY(candidate_curr->code));
#endif

    uint16_t code_to_match;
    if (candidate_curr->valid)
    {
        code_to_match = candidate_curr->next_code;
        uint16_t shifted = cyclic_lsl(code_to_match, 16);
#ifdef DEBUG
        printf(" "BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"\n",
                BYTE_TO_BINARY(code_to_match>>8), BYTE_TO_BINARY(code_to_match));
#endif
        if (match_even_odd(candidate_curr->code, code_to_match, NULL))
        {
            candidate_curr->next_code = cyclic_lsl(code_to_match, 16);
            candidate_curr->valid = 1;
        }
        // additional check in case code is matched to shifted version of itself
        else if (match_even_odd(candidate_curr->code, shifted, NULL))
        {
            candidate_curr->next_code = cyclic_lsl(shifted, 16);
            candidate_curr->valid = 1;
        }
        else {
#ifdef DEBUG
            printf("==== LOST ====\n");
            printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN" != ",
                    BYTE_TO_BINARY(candidate_curr->code>>8), BYTE_TO_BINARY(candidate_curr->code));
            printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN" | ",
                    BYTE_TO_BINARY(code_to_match>>8), BYTE_TO_BINARY(code_to_match));
            printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"\n",
                    BYTE_TO_BINARY(shifted>>8), BYTE_TO_BINARY(shifted));

            int j = candidate_curr->brightnesses.idx;
            for (int i = 0; i < BUF_SIZE; i++)
            {
                printf("%u ", candidate_curr->brightnesses.buf[j]);
                j = (j + 1) % BUF_SIZE;
            }
            puts("");
            printf("===============\n");
#endif
            candidate_curr->valid = 0;
            // candidate_curr->next_code = cyclic_lsl(code_to_match, 16);
        }
        return candidate_curr->valid;
    }
    else {
        for (int i = 0; i < zarray_size(ld->codes); i++)
        {
            glitter_code_t *code;
            zarray_get_volatile(ld->codes, i, &code);
            code_to_match = code->doubled_code;
#ifdef DEBUG
            printf(" "BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"\n",
                    BYTE_TO_BINARY(code_to_match>>8), BYTE_TO_BINARY(code_to_match));
#endif
            if (match_even_odd(candidate_curr->code, code_to_match, &candidate_curr->match_code))
            {
#ifdef DEBUG
                printf("==== MATCH ====\n");
                printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN" == ",
                        BYTE_TO_BINARY(candidate_curr->code>>8), BYTE_TO_BINARY(candidate_curr->code));
                printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"\n",
                        BYTE_TO_BINARY(code_to_match>>8), BYTE_TO_BINARY(code_to_match));
                printf("===============\n");
#endif
                candidate_curr->code = code_to_match;
                candidate_curr->next_code = cyclic_lsl(code_to_match, 16);
                candidate_curr->valid = 1;
                return 1;
            }
        }
        candidate_curr->valid = 0;
        return 0;
    }
}
