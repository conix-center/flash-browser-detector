#include "bit_match.h"
#include "linked_list.h"
#include "common/math_util.h"

#define EVEN_MASK       0xaaaa
#define ODD_MASK        0x5555

#define DEBUG

static inline size_t cyclic_lsr(size_t bits, size_t size)
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

size_t hamming_dist(size_t a, size_t b)
{
    size_t c = a ^ b;
    unsigned int hamm = 0;
    while (c) {
        ++hamm;
        c &= (c-1);
    }
    return hamm;
}

static int match_even_odd(uint16_t a, uint16_t b)
{
    uint16_t a_even = a & EVEN_MASK, a_odd = a & ODD_MASK;
    uint16_t b_even = b & EVEN_MASK, b_odd = b & ODD_MASK;
    return (a_even == b_even) || (a_odd == b_odd);
}

int match(lightanchor_detector_t *ld, lightanchor_t *candidate_curr)
{
#ifdef DEBUG
    printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN" ",
            BYTE_TO_BINARY(candidate_curr->code>>8), BYTE_TO_BINARY(candidate_curr->code));
#endif
    uint16_t match_code;
    if (candidate_curr->valid > 0) {
        match_code = candidate_curr->next_code;
#ifdef DEBUG
        printf(" "BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"\n",
                        BYTE_TO_BINARY(match_code>>8), BYTE_TO_BINARY(match_code));
#endif
        if (match_even_odd(candidate_curr->code, match_code)) {
            candidate_curr->next_code = cyclic_lsr(match_code, 16);
            candidate_curr->valid++;
            return 1;
        }
        else {
#ifdef DEBUG
            printf("==== LOST ====\n");
            printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN" != ",
                BYTE_TO_BINARY(candidate_curr->code>>8), BYTE_TO_BINARY(candidate_curr->code));
            printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"\n",
                    BYTE_TO_BINARY(match_code>>8), BYTE_TO_BINARY(match_code));

            struct ll_node *node = candidate_curr->brightnesses->head;
            for (; node != candidate_curr->brightnesses->tail; node = node->next) {
                printf(" %3d", node->data);
            }
            puts("");
            printf("===============\n");
#endif
            candidate_curr->next_code = 0;
            candidate_curr->valid = 0;
            return 0;
        }
    }
    else {
        match_code = ld->code;
#ifdef DEBUG
        printf(" "BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"\n",
                        BYTE_TO_BINARY(match_code>>8), BYTE_TO_BINARY(match_code));
#endif
        if (match_even_odd(candidate_curr->code, match_code)) {
#ifdef DEBUG
            printf("==== MATCH ====\n");
            printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN" == ",
                    BYTE_TO_BINARY(candidate_curr->code>>8), BYTE_TO_BINARY(candidate_curr->code));
            printf(""BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"\n",
                    BYTE_TO_BINARY(match_code>>8), BYTE_TO_BINARY(match_code));
            printf("===============\n");
#endif
            candidate_curr->code = match_code;
            candidate_curr->next_code = cyclic_lsr(match_code, 16);
            candidate_curr->valid++;
            return 1;
        }
        return 0;
    }
}

#if 0
static uint8_t dwt(uint16_t a, uint16_t b)
{
    // dynamic time warping with window of w
    static uint8_t dtw_matrix[17][17];
    int w = 3;

    for (int i = 0; i < 17; i++) {
        for (int j = 0; j < 17; j++) {
            dtw_matrix[i][j] = (uint8_t)255;
        }
    }
    dtw_matrix[0][0] = 0;

    for (int i = 0; i < 17; i++) {
        for (int j = imax(1,i-w); j < imin(16,i+w)+1; j++) {
            dtw_matrix[i][j] = 0;
        }
    }

    for (int i = 1; i < 17; i++) {
        for (int j = imax(1,i-w); j < imin(16,i+w)+1; j++) {
            uint8_t cost = !!(a & (1 << (15-(i-1)))) != !!(b & (1 << (15-(j-1))));
            uint8_t last_min = imin(dtw_matrix[i-1][j], dtw_matrix[i][j-1]);
            last_min = imin(last_min, dtw_matrix[i-1][j-1]);
            dtw_matrix[i][j] = cost + last_min;
        }
    }

    return !dtw_matrix[16][16];
}

int match_dtw(lightanchor_detector_t *ld, lightanchor_t *candidate_curr)
{
    uint16_t code = 0;
    uint8_t thres = ll_mid(candidate_curr->brightnesses), i = 15;

    struct ll_node *node = candidate_curr->brightnesses->head;
    for (; node != candidate_curr->brightnesses->tail; node = node->next) {
        code |= ((node->data > thres) << i--);
    }

    uint16_t match_code;
    if (candidate_curr->next_code) {
#ifdef DEBUG
        printf(" match "BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"",
            BYTE_TO_BINARY(candidate_curr->next_code>>8), BYTE_TO_BINARY(candidate_curr->next_code));
        printf(" > "BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"\n",
                BYTE_TO_BINARY(code>>8), BYTE_TO_BINARY(code));
#endif
        match_code = candidate_curr->next_code;
        if (dwt(code, match_code)) {
            candidate_curr->next_code = cyclic_lsr(candidate_curr->next_code, 16);
            return 1;
        }
        else {
            candidate_curr->next_code = 0;
            return 0;
        }
    }
    else {
        match_code = ld->code;
        if (dwt(code, match_code)) {
            candidate_curr->next_code = cyclic_lsr(match_code, 16);
#ifdef DEBUG
            printf(" find  "BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"",
                BYTE_TO_BINARY(ld->code>>8), BYTE_TO_BINARY(ld->code));
            printf(" > "BYTE_TO_BINARY_PATTERN""BYTE_TO_BINARY_PATTERN"\n",
                    BYTE_TO_BINARY(code>>8), BYTE_TO_BINARY(code));
#endif
            return 1;
        }
        return 0;
    }
}
#endif
