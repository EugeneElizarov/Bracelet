#include <stddef.h>
#include "ring_buffer.h"

typedef struct
{
    uint16_t size;
    uint16_t free;
    uint16_t w_index;
    uint16_t r_index;
    uint8_t buffer[2];
}RingBuffer_desc;

typedef RingBuffer_desc *RingBuffer;

Handle RingBuffer_Init(void *buffer, uint16_t buffer_size)
{
    if ((buffer == NULL) || (buffer_size <= sizeof(RingBuffer_desc)))
        return NULL;

    uint32_t addr = (uint32_t)buffer;

    while ((addr & 3) != 0)
    {
        addr++;
        buffer_size--;
    }

    if (buffer_size <= sizeof(RingBuffer_desc))
        return NULL;
    else
    {
        RingBuffer ring = (void *)addr;
        ring->size = buffer_size - sizeof(RingBuffer_desc) + sizeof(ring->buffer);
        RingBuffer_Clear(ring);
        return ring;
    }
}

static inline uint8_t *save_and_next(uint8_t *ptr, uint8_t *start, uint8_t *terminator, uint8_t data)
{
    *ptr = data;
    ptr++;
    if (ptr >= terminator)
        return start;
    return ptr;
}

static inline uint8_t read_and_next(uint8_t **ptr, uint8_t *start, uint8_t *terminator)
{
    uint8_t data = **ptr;

    *ptr = *ptr + 1;
    if (*ptr >= terminator)
        *ptr = start;
    return data;
}

bool RingBuffer_AddData(Handle ringbuffer, void *data, uint16_t size)
{
    if (ringbuffer)
    {
        RingBuffer ring = ringbuffer;

        if (ring->size >= (size + 2))
        {
            while (ring->free < (size + 2))
            {
                uint16_t bsize = RingBuffer_GetSize(ring);
                ring->r_index += bsize + 2;
                while (ring->r_index >= ring->size)
                    ring->r_index -= ring->size;
                ring->free += bsize + 2;
            }
            return RingBuffer_AddDataSafe(ringbuffer, data, size);
        }
    }
    return false;
}

bool RingBuffer_AddDataSafe(Handle ringbuffer, void *data, uint16_t size)
{
    if (ringbuffer)
    {
        RingBuffer ring = ringbuffer;

        if (ring->free >= (size + 2))
        {
            uint8_t *buffer = &(ring->buffer[ring->w_index]);
            uint8_t *terminator = &(ring->buffer[ring->size]);
            uint8_t *start = ring->buffer;
            uint8_t *source = data;
            uint16_t i;

            buffer = save_and_next(buffer, start, terminator, size & 0xFF);
            buffer = save_and_next(buffer, start, terminator, size >> 8);

            for (i = 0; i < size; i++)
                buffer = save_and_next(buffer, start, terminator, *source++);

            ring->free -= (size + 2);
            ring->w_index += size + 2;
            while (ring->w_index >= ring->size)
                ring->w_index -= ring->size;

            return true;
        }
    }
    return false;
}

uint16_t RingBuffer_GetSize(Handle ringbuffer)
{
    if (ringbuffer)
    {
        RingBuffer ring = ringbuffer;

        if (ring->r_index != ring->w_index)
        {
            uint8_t *buffer = &(ring->buffer[ring->r_index]);
            uint8_t *terminator = &(ring->buffer[ring->size]);
            uint8_t *start = ring->buffer;
            uint16_t value;
            uint8_t *data = (uint8_t *)&value;

            *data++ = read_and_next(&buffer, start, terminator);
            *data = read_and_next(&buffer, start, terminator);

            return value;
        }
    }
    return 0;
}

uint16_t RingBuffer_ReadData(Handle ringbuffer, void *dest, uint16_t dest_size)
{
    if ((ringbuffer != NULL) && (dest != NULL))
    {
        RingBuffer ring = ringbuffer;

        if (ring->r_index != ring->w_index)
        {
            uint8_t *buffer = &(ring->buffer[ring->r_index]);
            uint8_t *terminator = &(ring->buffer[ring->size]);
            uint8_t *start = ring->buffer;
            uint16_t size;
            uint8_t *data = (uint8_t *)&size;

            *data++ = read_and_next(&buffer, start, terminator);
            *data = read_and_next(&buffer, start, terminator);

            if ((size != 0) && (size <= dest_size))
            {
                uint8_t *dst = dest;
                uint16_t i = size;

                while (i--)
                    *dst++ = read_and_next(&buffer, start, terminator);

                return size;
            }
        }
    }
    return 0;
}

void RingBuffer_DeleteData(Handle ringbuffer)
{
    if (ringbuffer)
    {
        RingBuffer ring = ringbuffer;
        if (ring->r_index != ring->w_index)
        {
        	uint16_t size = RingBuffer_GetSize(ring);

        	ring->r_index += 2 + size;

        	while (ring->r_index >= ring->size)
        		ring->r_index -= ring->size;
        	ring->free += 2 + size;
        }
    }
}

uint16_t RingBuffer_GetData(Handle ringbuffer, void *dest, uint16_t dest_size)
{
	uint16_t size = RingBuffer_ReadData(ringbuffer, dest, dest_size);
	if (size)
		RingBuffer_DeleteData(ringbuffer);
	return size;
}

void RingBuffer_Clear(Handle ringbuffer)
{
    if (ringbuffer)
    {
        RingBuffer ring = ringbuffer;
        ring->free = ring->size;
        ring->r_index = 0;
        ring->w_index = 0;
    }
}
