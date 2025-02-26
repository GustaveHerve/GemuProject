#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stddef.h>

#define DEFINE_RING_BUFFER(TYPE, SIZE)                                                                                 \
    struct ring_buffer_##TYPE                                                                                          \
    {                                                                                                                  \
        TYPE buffer[(SIZE)];                                                                                           \
        size_t head;                                                                                                   \
        size_t tail;                                                                                                   \
        size_t element_count;                                                                                          \
    };                                                                                                                 \
                                                                                                                       \
    static inline void ring_buffer_##TYPE##_init(struct ring_buffer_##TYPE *ring_buffer)                               \
    {                                                                                                                  \
        ring_buffer->head = 0;                                                                                         \
        ring_buffer->tail = 0;                                                                                         \
        ring_buffer->element_count = 0;                                                                                \
    }                                                                                                                  \
                                                                                                                       \
    static inline void ring_buffer_##TYPE##_enqueue(struct ring_buffer_##TYPE *ring_buffer, TYPE *elt)                 \
    {                                                                                                                  \
        /* Overwrite the oldest element if necessary */                                                                \
        if (ring_buffer->element_count < (SIZE))                                                                       \
            ++ring_buffer->element_count;                                                                              \
        else                                                                                                           \
            ring_buffer->head = (ring_buffer->head + 1) % (SIZE);                                                      \
        ring_buffer->buffer[ring_buffer->tail] = *elt;                                                                 \
        ring_buffer->tail = (ring_buffer->tail + 1) % (SIZE);                                                          \
    }                                                                                                                  \
                                                                                                                       \
    static inline int ring_buffer_##TYPE##_dequeue(struct ring_buffer_##TYPE *ring_buffer, TYPE *output)               \
    {                                                                                                                  \
        if (ring_buffer->element_count == 0)                                                                           \
            return -1;                                                                                                 \
        --ring_buffer->element_count;                                                                                  \
        if (output)                                                                                                    \
            *output = ring_buffer->buffer[ring_buffer->head];                                                          \
        ring_buffer->head = (ring_buffer->head + 1) % (SIZE);                                                          \
        return 0;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    static inline size_t ring_buffer_##TYPE##_get_count(struct ring_buffer_##TYPE *ring_buffer)                        \
    {                                                                                                                  \
        return ring_buffer->element_count;                                                                             \
    }                                                                                                                  \
                                                                                                                       \
    static inline int ring_buffer_##TYPE##_get_front(struct ring_buffer_##TYPE *ring_buffer, TYPE *output)             \
    {                                                                                                                  \
        if (ring_buffer->element_count == 0 || !output)                                                                \
            return -1;                                                                                                 \
        *output = ring_buffer->buffer[ring_buffer->head];                                                              \
        return 0;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    static inline int ring_buffer_##TYPE##_get_rear(struct ring_buffer_##TYPE *ring_buffer, TYPE *output)              \
    {                                                                                                                  \
        if (ring_buffer->element_count == 0 || !output)                                                                \
            return -1;                                                                                                 \
        *output = ring_buffer->buffer[ring_buffer->tail - 1];                                                          \
        return 0;                                                                                                      \
    }                                                                                                                  \
                                                                                                                       \
    static inline size_t ring_buffer_##TYPE##_get_size(void)                                                           \
    {                                                                                                                  \
        return (SIZE);                                                                                                 \
    }

#define RING_BUFFER(TYPE) struct ring_buffer_##TYPE

#define RING_BUFFER_INIT(TYPE, BUFFER) ring_buffer_##TYPE##_init((BUFFER))

#define RING_BUFFER_ENQUEUE(TYPE, BUFFER, ELT) ring_buffer_##TYPE##_enqueue((BUFFER), (ELT))

#define RING_BUFFER_DEQUEUE(TYPE, BUFFER, OUTPUT) ring_buffer_##TYPE##_dequeue((BUFFER), (OUTPUT))

#define RING_BUFFER_GET_COUNT(TYPE, BUFFER) ring_buffer_##TYPE##_get_count((BUFFER))

#define RING_BUFFER_IS_EMPTY(TYPE, BUFFER) (ring_buffer_##TYPE##_get_count((BUFFER)) == 0)

#define RING_BUFFER_GET_FRONT(TYPE, BUFFER, OUTPUT) ring_buffer_##TYPE##_get_front((BUFFER), (OUTPUT))

#define RING_BUFFER_GET_REAR(TYPE, BUFFER, OUTPUT) ring_buffer_##TYPE##_get_rear((BUFFER), (OUTPUT))

#define RING_BUFFER_GET_SIZE(TYPE, BUFFER) ring_buffer_##TYPE##_get_size((BUFFER))

#endif
