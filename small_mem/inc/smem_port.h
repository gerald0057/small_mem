#ifndef __SMEM_PORT_H
#define __SMEM_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <assert.h>

#ifndef _ASSERT
    #define _ASSERT assert
#endif

#ifndef LOG_TAG
    #define LOG_TAG " "
#endif

#ifndef LOG_I
    #define LOG_I(...) do { printf("[I] "LOG_TAG" "); printf(__VA_ARGS__); } while (0)
    #define LOG_D(...) do { printf("[D] "LOG_TAG" "); printf(__VA_ARGS__); } while (0)
    #define LOG_E(...) do { printf("[E] "LOG_TAG" "); printf(__VA_ARGS__); } while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __SMEM_PORT_H */

