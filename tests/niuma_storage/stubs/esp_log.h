#pragma once
/* Logging cannot affect fault-injection state. */
#define ESP_LOGE(...) ((void)0)
