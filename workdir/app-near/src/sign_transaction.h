#include "os.h"
#include "cx.h"
#include "globals.h"

#ifndef _SIGN_TRANSACTION_H_
#define _SIGN_TRANSACTION_H_

void sign_ux_flow_init();

void handle_sign_transaction(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx);

#endif