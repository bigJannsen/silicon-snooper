#ifndef GUI_RINGBUFFER_H
#define GUI_RINGBUFFER_H

#include <stddef.h>

typedef struct {
    double *values;
    size_t capacity;
    size_t count;
    size_t head;
} GuiRingBuffer;

int gui_ring_buffer_init(GuiRingBuffer *buffer, size_t capacity);
void gui_ring_buffer_destroy(GuiRingBuffer *buffer);
void gui_ring_buffer_push(GuiRingBuffer *buffer, double value);
size_t gui_ring_buffer_copy(const GuiRingBuffer *buffer, double *out_values, size_t max_values);

#endif
