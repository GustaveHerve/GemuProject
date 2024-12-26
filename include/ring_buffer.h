#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <assert.h>
#include <stddef.h>

#define DEFINE_RING_BUFFER(TYPE, SIZE)                                                                                 \
    struct ring_buffer_##TYPE                                                                                          \
    {                                                                                                                  \
        TYPE buffer[(SIZE)];                                                                                           \
        size_t head;                                                                                                   \
        size_t tail;                                                                                                   \
        size_t element_count;                                                                                          \
    };                                                                                                                 \
    void ring_buffer_##TYPE##_init(struct ring_buffer_##TYPE *ring_buffer)                                             \
    {                                                                                                                  \
        ring_buffer->head = 0;                                                                                         \
        ring_buffer->tail = 0;                                                                                         \
        ring_buffer->element_count = 0;                                                                                \
    }                                                                                                                  \
                                                                                                                       \
    void ring_buffer_##TYPE##_enqueue(struct ring_buffer_##TYPE *ring_buffer, TYPE *elt)                               \
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
    int ring_buffer_##TYPE##_dequeue(struct ring_buffer_##TYPE *ring_buffer, TYPE *output)                             \
    {                                                                                                                  \
        /* Dequeuing if no element makes no sense so forbidden */                                                      \
        if (ring_buffer->element_count == 0)                                                                           \
            return -1;                                                                                                 \
        --ring_buffer->element_count;                                                                                  \
        *output = ring_buffer->buffer[ring_buffer->head];                                                              \
        ring_buffer->head = (ring_buffer->head + 1) % (SIZE);                                                          \
        return 0;                                                                                                      \
    }

#define RING_BUFFER_INIT(TYPE, BUFFER) ring_buffer_##TYPE##_init((BUFFER));

#define RING_BUFFER_ENQUEUE(TYPE, BUFFER, ELT) ring_buffer_##TYPE##_enqueue((BUFFER), (ELT));

#define RING_BUFFER_DEQUEUE(TYPE, BUFFER, OUTPUT) ring_buffer_##TYPE##_dequeue((BUFFER), (OUTPUT));

#endif
