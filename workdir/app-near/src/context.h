#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <stdint.h>

#include "constants.h"

// A place to store information about the transaction
// for displaying to the user when requesting approval
// 64 for address/id and +1 for \0
typedef struct uiContext_t {
	char line1[65];
	char line2[65];
	char line3[65];
	char line5[65];
	char amount[65];
	char long_line[250];
} uiContext_t;

// A place to store data during the signing
typedef struct signingContext_t {
    // bip32 path
	uint32_t bip32[5];
	char buffer[MAX_DATA_SIZE];
	uint32_t buffer_used;
	unsigned char network_byte;
} signingContext_t;

// A place to store data during the confirming the address
typedef struct addressesContext_t {
	char public_key[32];
} addressesContext_t;

typedef union {
    signingContext_t signing_context;
    addressesContext_t address_context;
} tmpContext_t;

extern uiContext_t ui_context;

extern tmpContext_t tmp_ctx; // Temporary area to store stuff

#endif /* __CONTEXT_H__ */