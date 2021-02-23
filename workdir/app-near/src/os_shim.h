#ifndef __OS_SHIM_H__
#define __OS_SHIM_H__

#ifdef OS_IO_SEPROXYHAL
    #include <os.h>
#else
    #include <stdlib.h>
    #define THROW(x) do { \
        printf("THROW(0x%x)\n", x); \
        exit(1); \
    } while (0);
#ifdef UNITTEST
    #define PRINTF(...)
#else
    #include <stdio.h>
    #define PRINTF printf
#endif // UNITTEST
#endif

#endif /* __OS_SHIM_H__ */