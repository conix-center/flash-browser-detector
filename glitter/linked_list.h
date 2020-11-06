#ifndef _LINKED_LIST_
#define _LINKED_LIST_

struct ll_node {
    struct ll_node *next;
    uint8_t data;
};

struct ll {
    struct ll_node *head;
    struct ll_node *tail;
    uint8_t capacity;
    uint8_t size;
};

static inline struct ll *ll_create(uint8_t capacity) {
    struct ll *res = calloc(1, sizeof(struct ll));
    res->capacity = capacity;
    return res;
}

static inline void ll_add(struct ll *list, uint8_t data) {
    struct ll_node *node = calloc(1, sizeof(struct ll_node));
    node->data = data;
    if (list->head == NULL && list->tail == NULL) {
        list->head = node;
        list->tail = list->head;
        list->size++;
    }
    else if (list->size <= list->capacity) {
        list->tail->next = node;
        list->tail = node;
        list->size++;
    }
    else {
        struct ll_node *temp = list->head;
        list->head = list->head->next;
        list->tail->next = node;
        list->tail = node;
        free(temp);
    }
}

static inline uint8_t ll_max(struct ll *list) {
    uint8_t max = 0;
    struct ll_node *node = list->head;
    for (; node != list->tail; node = node->next) {
        max = node->data > max ? node->data : max;
    }
    return max;
}

static inline uint8_t ll_min(struct ll *list) {
    uint8_t min = 255;
    struct ll_node *node = list->head;
    for (; node != list->tail; node = node->next) {
        min = node->data < min ? node->data : min;
    }
    return min;
}

static inline uint8_t ll_mid(struct ll *list) {
    uint8_t max = 0, min = 255;
    struct ll_node *node = list->head;
    for (; node != list->tail; node = node->next) {
        max = node->data > max ? node->data : max;
        min = node->data < min ? node->data : min;
    }
    return (max + min) / 2;
}

#endif
