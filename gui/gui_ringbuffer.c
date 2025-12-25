#include "gui_ringbuffer.h"
#include <stdlib.h>
#include <string.h>

int gui_ring_buffer_init(GuiRingBuffer *buffer, size_t capacity) {
    if (!buffer || capacity == 0) {
        return -1;
    }

    buffer->values = (double *)calloc(capacity, sizeof(double));
    if (!buffer->values) {
        return -1;
    }

    buffer->capacity = capacity;
    buffer->count = 0;
    buffer->head = 0;
    return 0;
}

void gui_ring_buffer_destroy(GuiRingBuffer *buffer) {
    if (!buffer) {
        return;
    }
    free(buffer->values);
    buffer->values = NULL;
    buffer->capacity = 0;
    buffer->count = 0;
    buffer->head = 0;
}

void gui_ring_buffer_push(GuiRingBuffer *buffer, double value) {
    if (!buffer || !buffer->values || buffer->capacity == 0) {
        return;
    }

    buffer->values[buffer->head] = value;
    buffer->head = (buffer->head + 1) % buffer->capacity;
    if (buffer->count < buffer->capacity) {
        buffer->count++;
    }
}

size_t gui_ring_buffer_copy(const GuiRingBuffer *buffer, double *out_values, size_t max_values) {
    if (!buffer || !out_values || buffer->count == 0 || max_values == 0) {
        return 0;
    }

    size_t to_copy = buffer->count < max_values ? buffer->count : max_values;
    size_t start = (buffer->head + buffer->capacity - buffer->count) % buffer->capacity;

    for (size_t i = 0; i < to_copy; ++i) {
        size_t index = (start + i) % buffer->capacity;
        out_values[i] = buffer->values[index];
    }

    return to_copy;
}

void gui_ring_buffer_clear(GuiRingBuffer *buffer) {
    if (!buffer || !buffer->values) {
        return;
    }
    buffer->count = 0;
    buffer->head = 0;
    memset(buffer->values, 0, sizeof(double) * buffer->capacity);
}
