#pragma once
#include <stddef.h>
typedef void *QueueHandle_t;
QueueHandle_t xQueueCreate(int count,size_t size);
int xQueueSend(QueueHandle_t q,const void *data,int timeout);
int xQueueReceive(QueueHandle_t q,void *data,int timeout);
