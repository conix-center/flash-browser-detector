/** @file queue_buf.h
 *  @brief Definitions for buffered queues
 *
 *  Buffered queues are arrays of fixed length (of BUF_SIZE) that can be
 *  enqueued and dequeued. When full, every enqueue will automatically dequeue
 *  an element.
 *
 * Copyright (C) Wiselab CMU.
 * @author Edward Lu (elu2@andrew.cmu.edu)
 * @date July, 2020
 */
#ifndef _QUEUE_BUF_H_
#define _QUEUE_BUF_H_

#define BUF_SIZE    16

struct queue_buf {
    int idx;
    int full;
    uint8_t buf[BUF_SIZE];
};

int qb_full(struct queue_buf *qb);
uint8_t qb_add(struct queue_buf *qb, uint8_t in);
void qb_stats(struct queue_buf *qb, uint8_t *max, uint8_t *min, uint8_t *avg);
void qb_copy(struct queue_buf *src, struct queue_buf *dest);

#endif
