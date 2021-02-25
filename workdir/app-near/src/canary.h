#ifndef _CANARY_H_
#define _CANARY_H_

#define PLOC() PRINTF("\nfile: %s function: %s line: %d \n", __FILE__, __func__, __LINE__);

#define FAIL(x)                                    \
    {                                              \
        PLOC();                                    \
        PRINTF("NEAR Ledger app failed: %s\n", x); \
        THROW(EXCEPTION);                          \
    }

#ifdef HAVE_BOLOS_APP_STACK_CANARY
/* This symbol is defined by the link script
to be at the start of the stack area */
extern unsigned long _stack, _estack;
#define STACK_CANARY (*((volatile uint32_t *)&_stack))

void print_available_stack();

#define INIT_CANARY                                                                     \
    STACK_CANARY = 0xDEADBEEF;                                                          \
    PLOC();                                                                             \
    PRINTF("CANNARY INITIALIZED \nSTACK_START 0x%p \nSTACK_END 0x%p \nSTACK_SIZE=%d\n", \
           &_stack, &_estack, ((uintptr_t)&_estack) - ((uintptr_t)&_stack));            \
    print_available_stack();

#define CHECK_CANARY                              \
    if (STACK_CANARY != 0xDEADBEEF)               \
        FAIL("CHECK_CANARY: EXCEPTION_OVERFLOW"); \
    PLOC();                                       \
    print_available_stack();                      \
    PRINTF("CHECK_CANARY: successfull\n");

#else

#define INIT_CANARY
#define CHECK_CANARY

#endif // HAVE_BOLOS_APP_STACK_CANARY

#endif