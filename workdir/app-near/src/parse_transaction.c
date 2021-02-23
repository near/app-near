#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "parse_transaction.h"

#include "context.h"
#include "os_shim.h"

/*
 Adapted from https://en.wikipedia.org/wiki/Double_dabble#C_implementation
 Returns: length of resulting string or -1 for error
*/
static int format_long_int_amount(size_t input_size, char *input, size_t output_size, char *output) {
    // NOTE: Have to copy to have word-aligned array (otherwise crashing on read)
    // Lots of time has been lost debugging this, make sure to avoid unaligned RAM access (as compiler in BOLOS SDK won't)
    uint16_t aligned_amount[8];
    memcpy(aligned_amount, input, 16);
    // Convert size in bytes into words
    size_t n = input_size / 2;

    size_t nbits = 16 * n;       /* length of arr in bits */
    size_t nscratch = nbits / 3; /* length of scratch in bytes */
    if (nscratch >= output_size) {
        // Output buffer is too small
        output[0] = '\0';
        return -1;
    }

    char *scratch = output;

    size_t i, j, k;
    size_t smin = nscratch - 2; /* speed optimization */

    for (i = 0; i < n; ++i) {
        for (j = 0; j < 16; ++j) {
            /* This bit will be shifted in on the right. */
            int shifted_in = (aligned_amount[n - i - 1] & (1 << (15 - j))) ? 1 : 0;

            /* Add 3 everywhere that scratch[k] >= 5. */
            for (k = smin; k < nscratch; ++k) {
                scratch[k] += (scratch[k] >= 5) ? 3 : 0;
            }

            /* Shift scratch to the left by one position. */
            if (scratch[smin] >= 8) {
                smin -= 1;
            }
            for (k = smin; k < nscratch - 1; ++k) {
                scratch[k] <<= 1;
                scratch[k] &= 0xF;
                scratch[k] |= (scratch[k + 1] >= 8);
            }

            /* Shift in the new bit from arr. */
            scratch[nscratch - 1] <<= 1;
            scratch[nscratch - 1] &= 0xF;
            scratch[nscratch - 1] |= shifted_in;
        }
    }

    /* Remove leading zeros from the scratch space. */
    for (k = 0; k < nscratch - 1; ++k) {
        if (scratch[k] != 0) {
            break;
        }
    }
    nscratch -= k;
    memmove(scratch, scratch + k, nscratch + 1);

    /* Convert the scratch space from BCD digits to ASCII. */
    for (k = 0; k < nscratch; ++k) {
        scratch[k] += '0';
    }

    /* Resize and return */
    memmove(output, scratch, nscratch + 1);
    return nscratch;
}

static int format_long_decimal_amount(size_t input_size, char *input, size_t output_size, char *output, int nomination) {
    int len = format_long_int_amount(input_size, input, output_size, output);

    if (len < 0 || (size_t) len + 2 > output_size) {
        // Output buffer is too small
        output[0] = '\0';
        return -1;
    }

    if (len <= nomination) {
        // < 1.0
        memmove(output + 2 + (nomination - len), output, len);
        /* coverity[bad_memset] */
        memset(output + 2, '0', (nomination - len));
        output[0] = '0';
        output[1] = '.';
        len = nomination + 2;
    } else {
        // >= 1.0
        int int_len = len - nomination;
        memmove(output + int_len + 1, output + int_len, nomination);
        output[int_len] = '.';
        len = len + 1;
    }


    // Remove trailing zeros
    output[len] = '0';
    while (len > 0 && output[len] == '0') {
        output[len--] = 0;
    }

    // Remove trailing dot
    if (output[len] == '.') {
        output[len] = 0;
    }


    return len;
}

static int check_overflow(unsigned int processed, unsigned int size) {
    PRINTF("check_overflow %d %d %d\n", processed, size, tmp_ctx.signing_context.buffer_used);
    if (size > tmp_ctx.signing_context.buffer_used || processed + size > tmp_ctx.signing_context.buffer_used) {
        return SIGN_PARSING_ERROR;
    }
    return 0;
}

#define PRINT_REMAINING_BUFFER() \
    PRINTF("remaining buffer: %.*h\n", tmp_ctx.signing_context.buffer_used - processed, &tmp_ctx.signing_context.buffer[processed]);

static int borsh_read_uint8(unsigned int *processed, uint8_t *n) {
    if (check_overflow(*processed, 1)) {
        return SIGN_PARSING_ERROR;
    }
    *n = *((uint8_t *) &tmp_ctx.signing_context.buffer[*processed]);
    *processed += 1;
    return 0;
}

static int borsh_read_uint32(unsigned int *processed, uint32_t *n) {
    if (check_overflow(*processed, 4)) {
        return SIGN_PARSING_ERROR;
    }
    *n = *((uint32_t *) &tmp_ctx.signing_context.buffer[*processed]);
    *processed += 4;
    return 0;
}

static int borsh_read_buffer(uint32_t *buffer_len, uint8_t **buffer, unsigned int *processed) {
    if (borsh_read_uint32(processed, buffer_len)) {
        return SIGN_PARSING_ERROR;
    }
    if (check_overflow(*processed, *buffer_len)) {
        return SIGN_PARSING_ERROR;
    }
    *buffer = &tmp_ctx.signing_context.buffer[*processed];
    *processed += *buffer_len;
    return 0;
}

static int borsh_read_fixed_buffer(unsigned int buffer_len, uint8_t **buffer, unsigned int *processed) {
    if (check_overflow(*processed, buffer_len)) {
        return SIGN_PARSING_ERROR;
    }
    *buffer = &tmp_ctx.signing_context.buffer[*processed];
    *processed += buffer_len;
    return 0;
}

static void strcpy_ellipsis(size_t dst_size, char *dst, size_t src_size, char *src) {
    if (dst_size >= src_size + 1) {
        memcpy(dst, src, src_size);
        dst[src_size] = 0;
        return;
    }

    memcpy(dst, src, dst_size);
    size_t ellipsis_start = dst_size >= 4 ? dst_size - 4 : 0;
    for (size_t i = ellipsis_start; i < dst_size; i++) {
        dst[i] = '.';
    }
    dst[dst_size - 1] = 0;
    return;
}

#define BORSH_SKIP(size) \
    if (check_overflow(processed, size)) { \
        return SIGN_PARSING_ERROR; \
    } \
    processed += size;

#define BORSH_DISPLAY_STRING(var_name, ui_line) \
    uint32_t var_name##_len; \
    char *var_name; \
    if (borsh_read_buffer(&var_name##_len, (uint8_t **) &var_name, &processed)) { \
        return SIGN_PARSING_ERROR; \
    } \
    strcpy_ellipsis(sizeof(ui_line), ui_line, var_name##_len, var_name); \
    PRINTF("%s: %s\n", #var_name, ui_line);

#define BORSH_DISPLAY_AMOUNT(var_name, ui_line) \
    if (check_overflow(processed, 16)) { \
        return SIGN_PARSING_ERROR; \
    } \
    char *var_name = (char *) &tmp_ctx.signing_context.buffer[processed]; \
    processed += 16; \
    format_long_decimal_amount(16, var_name, sizeof(ui_line), ui_line, 24);

#define COPY_LITERAL(dst, src) \
    memcpy(dst, src, sizeof(src))

typedef enum {
    at_create_account,
    at_deploy_contract,
    at_function_call,
    at_transfer,
    at_stake,
    at_add_key,
    at_delete_key,
    at_delete_account,
    at_last_value = at_delete_account
} action_type_t;

// Parse the transaction details for the user to approve
int parse_transaction() {
    memset(&ui_context, 0, sizeof(uiContext_t));

    // TODO: Validate data when parsing tx

    unsigned int processed = 0;

    // signer
    BORSH_DISPLAY_STRING(signer_id, ui_context.line3);

    // public key
    BORSH_SKIP(33);

    // nonce
    BORSH_SKIP(8);

    // receiver
    BORSH_DISPLAY_STRING(receiver_id, ui_context.line2);

    // block hash
    BORSH_SKIP(32);

    // actions
    uint32_t actions_len;
    if (borsh_read_uint32(&processed, &actions_len)) {
        return SIGN_PARSING_ERROR;
    }
    PRINTF("actions_len: %d\n", actions_len);

    if (actions_len != 1) {
        COPY_LITERAL(ui_context.line1, "multiple actions");
        return SIGN_FLOW_GENERIC;
    }

    // TODO: Parse more than one action

    // action type
    uint8_t action_type;
    if (borsh_read_uint8(&processed, &action_type)) {
        return SIGN_PARSING_ERROR;
    }
    PRINTF("action_type: %d\n", action_type);

    // TODO: assert action_type <= at_last_value

    switch (action_type) {
    case at_transfer: {
        COPY_LITERAL(ui_context.line1, "transfer");
        BORSH_DISPLAY_AMOUNT(amount, ui_context.amount);

        return SIGN_FLOW_TRANSFER;
    }

    case at_function_call: {
        // method name
        BORSH_DISPLAY_STRING(method_name, ui_context.line1);

        // args
        uint32_t args_len;
        char *args;
        if (borsh_read_buffer(&args_len, (uint8_t **) &args, &processed)) {
            return SIGN_PARSING_ERROR;
        }
        if (args_len > 0 && args[0] == '{') {
            // Args look like JSON
            strcpy_ellipsis(sizeof(ui_context.long_line), ui_context.long_line, args_len, args);
            // TODO: Make sure destination buffer is big enough
            PRINTF("args: %s\n", ui_context.long_line);
        } else {
            // TODO: Hexdump args otherwise
        }

        // gas
        BORSH_SKIP(8);

        // deposit
        BORSH_DISPLAY_AMOUNT(deposit, ui_context.line5);

        return SIGN_FLOW_FUNCTION_CALL;
    }

    case at_add_key: {
        COPY_LITERAL(ui_context.line1, "add key");
        // TODO: Assert that sender/receiver are the same?

        // public key

        // key type
        BORSH_SKIP(1);
        // TODO: assert ed25519 key type

        // key data
        uint8_t *key;
        if (borsh_read_fixed_buffer(32, &key, &processed)) {
            return SIGN_PARSING_ERROR;
        }
        // TODO: Display Base58 key?

        // access key

        // nonce
        BORSH_SKIP(8);

        // permission
        uint8_t permission_type;
        if (borsh_read_uint8(&processed, &permission_type)) {
            return SIGN_PARSING_ERROR;
        }
        PRINTF("permission_type: %d\n", permission_type);
        if (permission_type == 0) {
            // function call

            // allowance
            uint8_t has_allowance;
            if (borsh_read_uint8(&processed, &has_allowance)) {
                return SIGN_PARSING_ERROR;
            }
            if (has_allowance) {
                BORSH_DISPLAY_AMOUNT(allowance, ui_context.line5);
            } else {
                COPY_LITERAL(ui_context.line5, "Unlimited");
            }

            // receiver
            BORSH_DISPLAY_STRING(permission_receiver_id, ui_context.line2);

            // TODO: read method names array
            // TODO: Need to display one (multiple not supported yet – can just display "multiple methods")
            return SIGN_FLOW_ADD_FUNCTION_CALL_KEY;
        } else {
            // full access
            return SIGN_FLOW_ADD_FULL_ACCESS_KEY;
        }
    }

    case at_create_account: {
        COPY_LITERAL(ui_context.line1, "create account");
        // Use generic UI
        break;
    }

    case at_deploy_contract: {
        COPY_LITERAL(ui_context.line1, "deploy contract");
        // Use generic UI
        break;
    }

    case at_stake: {
        COPY_LITERAL(ui_context.line1, "stake");
        // Use generic UI
        break;
    }

    case at_delete_key: {
        COPY_LITERAL(ui_context.line1, "delete key");
        // Use generic UI
        break;
    }

    case at_delete_account: {
        COPY_LITERAL(ui_context.line1, "delete account");
        // Use generic UI
        break;
    }

    default:
        // TODO: Return more specific error?
        return SIGN_PARSING_ERROR;
    } // switch

    PRINT_REMAINING_BUFFER();

    return SIGN_FLOW_GENERIC;
}