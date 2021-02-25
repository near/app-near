#include "os.h"
#include "canary.h"

void print_available_stack()
{
    uint32_t stack_top = 0;
    
    PRINTF("CURRENT_STACK_ADDRESS: 0x%p \nSTACK_LIMIT: 0x%p \nAVAILABLE STACK: %d\n",
           &stack_top,
           &_stack,
           ((uintptr_t)&stack_top) - ((uintptr_t)&_stack));
}