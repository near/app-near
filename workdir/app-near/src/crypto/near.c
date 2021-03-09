#include "near.h"
#include "ledger_crypto.h"

void near_message_sign(const cx_ecfp_private_key_t *private_key, const unsigned char *message, const size_t message_size, ed25519_signature signature) {
    uint8_t hash[32]; 
    sha_256(message, message_size, hash);
    cx_eddsa_sign(private_key, 0, CX_SHA512, hash, sizeof(hash), NULL, 0, signature, 64, NULL);
}