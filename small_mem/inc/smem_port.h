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

#define LOG_ENABLE (0)

#if LOG_ENABLE
    #define LOG_D(...) do { printf("[D] "LOG_TAG" "); printf(__VA_ARGS__); } while (0)
    #define LOG_I(...) do { printf("[I] "LOG_TAG" "); printf(__VA_ARGS__); } while (0)
    #define LOG_E(...) do { printf("[E] "LOG_TAG" "); printf(__VA_ARGS__); } while (0)
#else
    #define LOG_D(...)
    #define LOG_I(...)
    #define LOG_E(...)
#endif

#define SMEM_ALIGN_SIZE (8)

#ifdef __cplusplus
}
#endif

#endif /* __SMEM_PORT_H */

