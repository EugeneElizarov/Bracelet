#ifndef __RING_BUFFER_H__

#define __RING_BUFFER_H__

#include <stdbool.h>
#include <stdint.h>

typedef void * Handle;

/* Инициализация кольцевого буфера */
Handle RingBuffer_Init(void *buffer, uint16_t buffer_size);
/* Добавление данных с удалением старых данных, если не хватает места*/
bool RingBuffer_AddData(Handle ringbuffer, void *data, uint16_t size);
/* Добавление данных, только если хватает места */
bool RingBuffer_AddDataSafe(Handle ringbuffer, void *data, uint16_t size);
/* Получение размера текущей записи */
uint16_t RingBuffer_GetSize(Handle ringbuffer);
/* Чтение текущей записи с удалением */
uint16_t RingBuffer_GetData(Handle ringbuffer, void *dest, uint16_t dest_size);
/* Чтение текущей записи без удаления */
uint16_t RingBuffer_ReadData(Handle ringbuffer, void *dest, uint16_t dest_size);
/* Удаление текущей записи */
void RingBuffer_DeleteData(Handle ringbuffer);
/* Полная очистка буфера */
void RingBuffer_Clear(Handle ringbuffer);

#endif
