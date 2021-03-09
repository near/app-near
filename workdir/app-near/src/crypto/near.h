#ifndef __WAVES_H__
#define __WAVES_H__

#include <stdint.h>
#include <stdbool.h>

#include "os.h"
#include "cx.h"

typedef unsigned char ed25519_signature[64];
typedef unsigned char ed25519_public_key[32];
typedef unsigned char ed25519_secret_key[32];

void near_message_sign(const cx_ecfp_private_key_t *private_key, const unsigned char *message, const size_t message_size, ed25519_signature signature);

#endif
