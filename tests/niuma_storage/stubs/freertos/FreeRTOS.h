#pragma once
#include <stdint.h>
#define pdTRUE 1
#define pdPASS 1
#define pdMS_TO_TICKS(n) (n)
typedef uint32_t TickType_t;
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(p) ((void)(p))
#define portEXIT_CRITICAL(p) ((void)(p))
