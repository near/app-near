#include "os.h"
#include "canary.h"

void print_available_stack()
{
    uint32_t stack_top = 0;
    PRINTF("Stack remaining: CUR_STACK_ADDR: 0x%p, STACK_LIMIT: 0x%p, Available: %d\n",
           &stack_top, &_stack, ((uintptr_t)&stack_top) - ((uintptr_t)&_stack));
}