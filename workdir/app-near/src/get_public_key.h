#include "os.h"
#include "cx.h"
#include "globals.h"

#ifndef _GET_PUBLIC_KEY_H_
#define _GET_PUBLIC_KEY_H_

void handle_get_public_key(uint8_t p1, uint8_t p2, const uint8_t *input_buffer, uint16_t input_length, volatile unsigned int *flags, volatile unsigned int *tx);

#endif
