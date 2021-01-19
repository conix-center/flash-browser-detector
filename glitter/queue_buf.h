#ifndef _QUEUE_BUF_
#define _QUEUE_BUF_

#define BUF_SIZE    16

struct queue_buf {
    int idx;
    uint8_t buf[BUF_SIZE];
};

static inline uint8_t qb_add(struct queue_buf *qb, uint8_t in) {
    uint8_t res = qb->buf[qb->idx];
    qb->buf[qb->idx] = in;
    qb->idx = (qb->idx + 1) % BUF_SIZE;
    return res;
}

static inline void qb_stats(struct queue_buf *qb, uint8_t *max, uint8_t *min, uint8_t *avg) {
    if (max) *max = 0;
    if (min) *min = 255;
    int sum = 0;
    for (int i = 0; i < BUF_SIZE; i++)
    {
        uint8_t val = qb->buf[i];
        if (max) *max = (val > *max) ? val : *max;
        if (min) *min = (val < *min) ? val : *min;
        if (avg) sum += val;
    }
    if (max) *max = (*max != 0) ? *max : 255;
    if (min) *min = (*min != 255) ? *min : 0;
    if (avg) *avg = sum / BUF_SIZE;
}

static inline void qb_copy(struct queue_buf *src, struct queue_buf *dest) {
    memcpy(src, dest, sizeof(struct queue_buf));
}

#endif
