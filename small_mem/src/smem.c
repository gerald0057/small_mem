/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2008-7-12      Bernard      the first version
 * 2010-06-09     Bernard      fix the end stub of heap
 *                             fix memory check in rt_realloc function
 * 2010-07-13     Bernard      fix _ALIGN issue found by kuronca
 * 2010-10-14     Bernard      fix rt_realloc issue when realloc a NULL pointer.
 * 2017-07-14     armink       fix rt_realloc issue when new size is 0
 * 2018-10-02     Bernard      Add 64bit support
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *         Simon Goldschmidt
 *
 */
#define LOG_TAG "[SMEM]"

#include <string.h>
#include "smem.h"
#include "smem_port.h"

#define MIN_SIZE (sizeof(uintptr_t) + sizeof(size_t) + sizeof(size_t))

#define MEM_MASK ((~(size_t)0) - 1)

/**
 * @def _ALIGN(size, align)
 * Return the most contiguous size aligned at specified width. _ALIGN(13, 4)
 * would return 16.
 */
#define _ALIGN(size, align)           (((size) + (align) - 1) & ~((align) - 1))

/**
 * @ingroup group_basic_definition
 *
 * @def _ALIGN_DOWN(size, align)
 * Return the down number of aligned at specified width. _ALIGN_DOWN(13, 4)
 * would return 12.
 * @note align Must be an integer power of 2 or the result will be incorrect
 */
#define _ALIGN_DOWN(size, align)      ((size) & ~((align) - 1))

#define ALIGN_SIZE (4)

#define MEM_USED(_mem) ((((uintptr_t)(_mem)) & MEM_MASK) | 0x1)
#define MEM_FREED(_mem) ((((uintptr_t)(_mem)) & MEM_MASK) | 0x0)
#define MEM_ISUSED(_mem) (((uintptr_t)(((struct small_mem_item *)(_mem))->pool_ptr)) & (~MEM_MASK))
#define MEM_POOL(_mem) ((struct small_mem *)(((uintptr_t)(((struct small_mem_item *)(_mem))->pool_ptr)) & (MEM_MASK)))
#define MEM_SIZE(_heap, _mem)                                                                                          \
    (((struct small_mem_item *)(_mem))->next - ((uintptr_t)(_mem) - (uintptr_t)((_heap)->heap_ptr)) -                  \
     _ALIGN(sizeof(struct small_mem_item), ALIGN_SIZE))

#define MIN_SIZE_ALIGNED _ALIGN(MIN_SIZE, ALIGN_SIZE)
#define SIZEOF_STRUCT_MEM _ALIGN(sizeof(struct small_mem_item), ALIGN_SIZE)

static void plug_holes(struct small_mem *m, struct small_mem_item *mem)
{
    struct small_mem_item *nmem;
    struct small_mem_item *pmem;

    _ASSERT((uint8_t *)mem >= m->heap_ptr);
    _ASSERT((uint8_t *)mem < (uint8_t *)m->heap_end);

    /* plug hole forward */
    nmem = (struct small_mem_item *)&m->heap_ptr[mem->next];
    if (mem != nmem && !MEM_ISUSED(nmem) && (uint8_t *)nmem != (uint8_t *)m->heap_end)
    {
        /* if mem->next is unused and not end of m->heap_ptr,
         * combine mem and mem->next
         */
        if (m->lfree == nmem)
        {
            m->lfree = mem;
        }
        nmem->pool_ptr = 0;
        mem->next = nmem->next;
        ((struct small_mem_item *)&m->heap_ptr[nmem->next])->prev = (uint8_t *)mem - m->heap_ptr;
    }

    /* plug hole backward */
    pmem = (struct small_mem_item *)&m->heap_ptr[mem->prev];
    if (pmem != mem && !MEM_ISUSED(pmem))
    {
        /* if mem->prev is unused, combine mem and mem->prev */
        if (m->lfree == mem)
        {
            m->lfree = pmem;
        }
        mem->pool_ptr = 0;
        pmem->next = mem->next;
        ((struct small_mem_item *)&m->heap_ptr[mem->next])->prev = (uint8_t *)pmem - m->heap_ptr;
    }
}

/**
 * @brief This function will initialize small memory management algorithm.
 *
 * @param begin_addr the beginning address of memory.
 *
 * @param size is the size of the memory.
 *
 * @return Return a pointer to the memory object. When the return value is NULL, it means the init failed.
 */
smem_t smem_init(void *begin_addr, size_t size)
{
    struct small_mem_item *mem;
    struct small_mem *small_mem;
    uintptr_t staaddr, begin_align, end_align, mem_size;

    small_mem = (struct small_mem *)_ALIGN((uintptr_t)begin_addr, ALIGN_SIZE);
    staaddr = (uintptr_t)small_mem + sizeof(*small_mem);
    begin_align = _ALIGN((uintptr_t)staaddr, ALIGN_SIZE);
    end_align = _ALIGN_DOWN((uintptr_t)begin_addr + size, ALIGN_SIZE);

    /* alignment addr */
    if ((end_align > (2 * SIZEOF_STRUCT_MEM)) && ((end_align - 2 * SIZEOF_STRUCT_MEM) >= staaddr))
    {
        /* calculate the aligned memory size */
        mem_size = end_align - begin_align - 2 * SIZEOF_STRUCT_MEM;
    }
    else
    {
        LOG_E("mem init, error begin address 0x%lx, and end address 0x%lx\r\n", (uintptr_t)begin_addr,
                (uintptr_t)begin_addr + size);

        return NULL;
    }

    memset(small_mem, 0, sizeof(*small_mem));
    /* initialize small memory object */
    small_mem->parent.address = begin_align;
    small_mem->parent.total = mem_size;
    small_mem->mem_size_aligned = mem_size;

    /* point to begin address of heap */
    small_mem->heap_ptr = (uint8_t *)begin_align;

    LOG_D("mem init, heap begin address 0x%lx, size %ld\r\n", (uintptr_t)small_mem->heap_ptr, small_mem->mem_size_aligned);

    /* initialize the start of the heap */
    mem = (struct small_mem_item *)small_mem->heap_ptr;
    mem->pool_ptr = MEM_FREED(small_mem);
    mem->next = small_mem->mem_size_aligned + SIZEOF_STRUCT_MEM;
    mem->prev = 0;

    /* initialize the end of the heap */
    small_mem->heap_end = (struct small_mem_item *)&small_mem->heap_ptr[mem->next];
    small_mem->heap_end->pool_ptr = MEM_USED(small_mem);
    small_mem->heap_end->next = small_mem->mem_size_aligned + SIZEOF_STRUCT_MEM;
    small_mem->heap_end->prev = small_mem->mem_size_aligned + SIZEOF_STRUCT_MEM;

    /* initialize the lowest-free pointer to the start of the heap */
    small_mem->lfree = (struct small_mem_item *)small_mem->heap_ptr;

    return (smem_t)(&small_mem->parent);
}

/**
 * @addtogroup group_memory_management
 */

/**@{*/

/**
 * @brief Allocate a block of memory with a minimum of 'size' bytes.
 *
 * @param m the small memory management object.
 *
 * @param size is the minimum size of the requested block in bytes.
 *
 * @return the pointer to allocated memory or NULL if no free memory was found.
 */
void *smem_alloc(smem_t m, size_t size)
{
    size_t ptr, ptr2;
    struct small_mem_item *mem, *mem2;
    struct small_mem *small_mem;

    if (size == 0)
        return NULL;

    _ASSERT(m != NULL);

    small_mem = (struct small_mem *)m;
    /* alignment size */
    size = _ALIGN(size, ALIGN_SIZE);

    /* every data block must be at least MIN_SIZE_ALIGNED long */
    if (size < MIN_SIZE_ALIGNED)
        size = MIN_SIZE_ALIGNED;

    if (size > small_mem->mem_size_aligned)
    {
        LOG_D("no memory\r\n");
        return NULL;
    }

    for (ptr = (uint8_t *)small_mem->lfree - small_mem->heap_ptr; ptr <= small_mem->mem_size_aligned - size;
         ptr = ((struct small_mem_item *)&small_mem->heap_ptr[ptr])->next)
    {
        mem = (struct small_mem_item *)&small_mem->heap_ptr[ptr];

        if ((!MEM_ISUSED(mem)) && (mem->next - (ptr + SIZEOF_STRUCT_MEM)) >= size)
        {
            /* mem is not used and at least perfect fit is possible:
             * mem->next - (ptr + SIZEOF_STRUCT_MEM) gives us the 'user data size' of mem */

            if (mem->next - (ptr + SIZEOF_STRUCT_MEM) >= (size + SIZEOF_STRUCT_MEM + MIN_SIZE_ALIGNED))
            {
                /* (in addition to the above, we test if another struct small_mem_item (SIZEOF_STRUCT_MEM) containing
                 * at least MIN_SIZE_ALIGNED of data also fits in the 'user data space' of 'mem')
                 * -> split large block, create empty remainder,
                 * remainder must be large enough to contain MIN_SIZE_ALIGNED data: if
                 * mem->next - (ptr + (2*SIZEOF_STRUCT_MEM)) == size,
                 * struct small_mem_item would fit in but no data between mem2 and mem2->next
                 * @todo we could leave out MIN_SIZE_ALIGNED. We would create an empty
                 *       region that couldn't hold data, but when mem->next gets freed,
                 *       the 2 regions would be combined, resulting in more free memory
                 */
                ptr2 = ptr + SIZEOF_STRUCT_MEM + size;

                /* create mem2 struct */
                mem2 = (struct small_mem_item *)&small_mem->heap_ptr[ptr2];
                mem2->pool_ptr = MEM_FREED(small_mem);
                mem2->next = mem->next;
                mem2->prev = ptr;

                /* and insert it between mem and mem->next */
                mem->next = ptr2;

                if (mem2->next != small_mem->mem_size_aligned + SIZEOF_STRUCT_MEM)
                {
                    ((struct small_mem_item *)&small_mem->heap_ptr[mem2->next])->prev = ptr2;
                }
                small_mem->parent.used += (size + SIZEOF_STRUCT_MEM);
                if (small_mem->parent.max < small_mem->parent.used)
                    small_mem->parent.max = small_mem->parent.used;
            }
            else
            {
                /* (a mem2 struct does no fit into the user data space of mem and mem->next will always
                 * be used at this point: if not we have 2 unused structs in a row, plug_holes should have
                 * take care of this).
                 * -> near fit or excact fit: do not split, no mem2 creation
                 * also can't move mem->next directly behind mem, since mem->next
                 * will always be used at this point!
                 */
                small_mem->parent.used += mem->next - ((uint8_t *)mem - small_mem->heap_ptr);
                if (small_mem->parent.max < small_mem->parent.used)
                    small_mem->parent.max = small_mem->parent.used;
            }
            /* set small memory object */
            mem->pool_ptr = MEM_USED(small_mem);

            if (mem == small_mem->lfree)
            {
                /* Find next free block after mem and update lowest free pointer */
                while (MEM_ISUSED(small_mem->lfree) && small_mem->lfree != small_mem->heap_end)
                    small_mem->lfree = (struct small_mem_item *)&small_mem->heap_ptr[small_mem->lfree->next];

                _ASSERT(((small_mem->lfree == small_mem->heap_end) || (!MEM_ISUSED(small_mem->lfree))));
            }
            _ASSERT((uintptr_t)mem + SIZEOF_STRUCT_MEM + size <= (uintptr_t)small_mem->heap_end);
            _ASSERT((uintptr_t)((uint8_t *)mem + SIZEOF_STRUCT_MEM) % ALIGN_SIZE == 0);
            _ASSERT((((uintptr_t)mem) & (ALIGN_SIZE - 1)) == 0);

            LOG_I("allocate memory at 0x%lx, size: %ld\r\n", (uintptr_t)((uint8_t *)mem + SIZEOF_STRUCT_MEM),
                  (uintptr_t)(mem->next - ((uint8_t *)mem - small_mem->heap_ptr)));

            /* return the memory data except mem struct */
            return (uint8_t *)mem + SIZEOF_STRUCT_MEM;
        }
    }

    return NULL;
}

/**
 * @brief This function will change the size of previously allocated memory block.
 *
 * @param m the small memory management object.
 *
 * @param rmem is the pointer to memory allocated by mem_alloc.
 *
 * @param newsize is the required new size.
 *
 * @return the changed memory block address.
 */
void *smem_realloc(smem_t m, void *rmem, size_t newsize)
{
    size_t size;
    size_t ptr, ptr2;
    struct small_mem_item *mem, *mem2;
    struct small_mem *small_mem;
    void *nmem;

    _ASSERT(m != NULL);

    small_mem = (struct small_mem *)m;
    /* alignment size */
    newsize = _ALIGN(newsize, ALIGN_SIZE);
    if (newsize > small_mem->mem_size_aligned)
    {
        LOG_D("realloc: out of memory\r\n");
        return NULL;
    }
    else if (newsize == 0)
    {
        smem_free(rmem);
        return NULL;
    }

    /* allocate a new memory block */
    if (rmem == NULL)
        return smem_alloc((smem_t)(&small_mem->parent), newsize);

    _ASSERT((((uintptr_t)rmem) & (ALIGN_SIZE - 1)) == 0);
    _ASSERT((uint8_t *)rmem >= (uint8_t *)small_mem->heap_ptr);
    _ASSERT((uint8_t *)rmem < (uint8_t *)small_mem->heap_end);

    mem = (struct small_mem_item *)((uint8_t *)rmem - SIZEOF_STRUCT_MEM);

    /* current memory block size */
    ptr = (uint8_t *)mem - small_mem->heap_ptr;
    size = mem->next - ptr - SIZEOF_STRUCT_MEM;
    if (size == newsize)
    {
        /* the size is the same as */
        return rmem;
    }

    if (newsize + SIZEOF_STRUCT_MEM + MIN_SIZE < size)
    {
        /* split memory block */
        small_mem->parent.used -= (size - newsize);

        ptr2 = ptr + SIZEOF_STRUCT_MEM + newsize;
        mem2 = (struct small_mem_item *)&small_mem->heap_ptr[ptr2];
        mem2->pool_ptr = MEM_FREED(small_mem);
        mem2->next = mem->next;
        mem2->prev = ptr;
        mem->next = ptr2;
        if (mem2->next != small_mem->mem_size_aligned + SIZEOF_STRUCT_MEM)
        {
            ((struct small_mem_item *)&small_mem->heap_ptr[mem2->next])->prev = ptr2;
        }

        if (mem2 < small_mem->lfree)
        {
            /* the splited struct is now the lowest */
            small_mem->lfree = mem2;
        }

        plug_holes(small_mem, mem2);

        return rmem;
    }

    /* expand memory */
    nmem = smem_alloc((smem_t)(&small_mem->parent), newsize);
    if (nmem != NULL) /* check memory */
    {
        memcpy(nmem, rmem, size < newsize ? size : newsize);
        smem_free(rmem);
    }

    return nmem;
}

/**
 * @brief This function will release the previously allocated memory block by
 *        mem_alloc. The released memory block is taken back to system heap.
 *
 * @param rmem the address of memory which will be released.
 */
void smem_free(void *rmem)
{
    struct small_mem_item *mem;
    struct small_mem *small_mem;

    if (rmem == NULL)
        return;

    _ASSERT((((uintptr_t)rmem) & (ALIGN_SIZE - 1)) == 0);

    /* Get the corresponding struct small_mem_item ... */
    mem = (struct small_mem_item *)((uint8_t *)rmem - SIZEOF_STRUCT_MEM);
    /* ... which has to be in a used state ... */
    small_mem = MEM_POOL(mem);
    _ASSERT(small_mem != NULL);
    _ASSERT(MEM_ISUSED(mem));
    _ASSERT((uint8_t *)rmem >= (uint8_t *)small_mem->heap_ptr && (uint8_t *)rmem < (uint8_t *)small_mem->heap_end);
    _ASSERT(MEM_POOL(&small_mem->heap_ptr[mem->next]) == small_mem);

    LOG_D("release memory 0x%lx, size: %ld\r\n", (uintptr_t)rmem,
          (uintptr_t)(mem->next - ((uint8_t *)mem - small_mem->heap_ptr)));

    /* ... and is now unused. */
    mem->pool_ptr = MEM_FREED(small_mem);

    if (mem < small_mem->lfree)
    {
        /* the newly freed struct is now the lowest */
        small_mem->lfree = mem;
    }

    small_mem->parent.used -= (mem->next - ((uint8_t *)mem - small_mem->heap_ptr));

    /* finally, see if prev or next are free also */
    plug_holes(small_mem, mem);
}

/**@}*/

