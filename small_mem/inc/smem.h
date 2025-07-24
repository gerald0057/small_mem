#ifndef __SMEM_H
#define __SMEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/**
 * @def SMEM_ALIGN(size, align)
 * Return the most contiguous size aligned at specified width. SMEM_ALIGN(13, 4)
 * would return 16.
 */
#define SMEM_ALIGN(size, align)           (((size) + (align) - 1) & ~((align) - 1))

/**
 * @ingroup group_basic_definition
 *
 * @def SMEM_ALIGN_DOWN(size, align)
 * Return the down number of aligned at specified width. SMEM_ALIGN_DOWN(13, 4)
 * would return 12.
 * @note align Must be an integer power of 2 or the result will be incorrect
 */
#define SMEM_ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))

/*
 * memory structure
 */
struct memory
{
    uint32_t address; /**< memory start address */
    size_t total;     /**< memory size */
    size_t used;      /**< size used */
    size_t max;       /**< maximum usage */
};

struct small_mem_item
{
    uintptr_t pool_ptr; /**< small memory object addr */
    size_t next;        /**< next free item */
    size_t prev;        /**< prev free item */
};

/**
 * Base structure of small memory object
 */
struct small_mem
{
    struct memory parent; /**< inherit from memory */
    uint8_t *heap_ptr;    /**< pointer to the heap */
    struct small_mem_item *heap_end;
    struct small_mem_item *lfree;
    size_t mem_size_aligned; /**< aligned memory size */
};
typedef struct small_mem *smem_t;

smem_t smem_init(void *begin_addr, size_t size);
void *smem_alloc(smem_t m, size_t size);
void *smem_realloc(smem_t m, void *rmem, size_t newsize);
void smem_free(void *rmem);

#ifdef __cplusplus
}
#endif

#endif /* __SMEM_H */

