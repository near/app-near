#include "parse_transaction.h"
#include "context.h"

#include <stdio.h>
#include <stddef.h>
#include <string.h>

uiContext_t ui_context;
tmpContext_t tmp_ctx;

static void print_ui() {
    printf("---\n");
    printf("%s\n", ui_context.line1);
    printf("%s\n", ui_context.line2);
    printf("%s\n", ui_context.line3);
    printf("%s\n", ui_context.long_line);
    printf("%s\n", ui_context.line5);
    printf("%s\n", ui_context.amount);
}

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    memset(&ui_context, 0, sizeof(uiContext_t));

    if (Size > MAX_DATA_SIZE) {
        return 0;
    }
    memcpy(tmp_ctx.signing_context.buffer, Data, Size);
    tmp_ctx.signing_context.buffer_used = Size;
    parse_transaction();
    print_ui();
    return 0;
}
