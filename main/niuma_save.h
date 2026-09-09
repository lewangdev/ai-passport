#pragma once
#include "niuma_model.h"
#include <stddef.h>

#define NM_SAVE_CAPACITY 256
/* Versioned little-endian wire format with CRC; never persist C padding. */
size_t nm_save_encode(const nm_state_t *s, uint8_t *out, size_t capacity);
bool nm_save_decode(nm_state_t *s, const uint8_t *data, size_t length);
