#pragma once
#include <stddef.h>
typedef void *QueueHandle_t;
QueueHandle_t xQueueCreate(unsigned count,size_t size);
void vQueueDelete(QueueHandle_t q);
int xQueueReceive(QueueHandle_t q,void *out,unsigned timeout);
int xQueueOverwrite(QueueHandle_t q,const void *in);
unsigned uxQueueMessagesWaiting(QueueHandle_t q);
