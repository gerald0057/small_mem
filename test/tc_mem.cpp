/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-10-14     tyx          the first version
 */

#include <stdlib.h>
#include <iostream>
#include <chrono>
#include <gtest/gtest.h>
#include <small_mem/inc/smem.h>
#include <small_mem/inc/smem_port.h>
#include "list.h"

#define MEM_SIZE(_heap, _mem)      \
    (((struct small_mem_item *)(_mem))->next - ((unsigned long)(_mem) - \
    (unsigned long)((_heap)->heap_ptr)) - SMEM_ALIGN(sizeof(struct small_mem_item), SMEM_ALIGN_SIZE))

#define TEST_MEM_SIZE 1024

class SmallMemTest : public testing::Test
{
protected:
    struct mem_test_context
    {
        void *ptr;
        size_t size;
        uint8_t magic;
    };

    void SetUp() override {}
    void TearDown() override {}

    auto now(void)
    {
        return std::chrono::steady_clock::now();
    }

    int _mem_cmp(void *ptr, uint8_t v, size_t size)
    {
        while (size-- != 0)
        {
            if (*(uint8_t *)ptr != v)
                return *(uint8_t *)ptr - v;
        }
        return 0;
    }

    size_t max_block(struct small_mem *heap)
    {
        struct small_mem_item *mem;
        size_t max = 0, size;

        for (mem = (struct small_mem_item *)heap->heap_ptr;
            mem != heap->heap_end;
            mem = (struct small_mem_item *)&heap->heap_ptr[mem->next])
        {
            if (((unsigned long)mem->pool_ptr & 0x1) == 0)
            {
                size = MEM_SIZE(heap, mem);
                if (size > max)
                {
                    max = size;
                }
            }
        }
        return max;
    }

    void progress(double percentage)
    {
        int barWidth = 50;
        std::cout << "[";
        int pos = barWidth * percentage;
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << int(percentage * 100.0) << " %\r";
        std::cout.flush();
    }
};

TEST_F(SmallMemTest, mem_functional_test)
{
    size_t total_size;
    uint8_t *buf;
    struct small_mem *heap;
    uint8_t magic = __LINE__;

    /* Prepare test memory */
    buf = (uint8_t *)malloc(TEST_MEM_SIZE);
    EXPECT_NE(buf, nullptr);
    EXPECT_EQ(SMEM_ALIGN((unsigned long)buf, SMEM_ALIGN_SIZE), (unsigned long)buf);
    memset(buf, 0xAA, TEST_MEM_SIZE);
    /* small heap init */
    heap = (struct small_mem *)smem_init(buf, TEST_MEM_SIZE);
    /* get total size */
    total_size = max_block(heap);
    EXPECT_NE(total_size, 0);
    /*
     * Allocate all memory at a time and test whether
     * the memory allocation release function is effective
     */
    {
        struct mem_test_context ctx;
        ctx.magic = magic++;
        ctx.size = max_block(heap);
        ctx.ptr = smem_alloc(heap, ctx.size);
        EXPECT_NE(ctx.ptr, nullptr);
        memset(ctx.ptr, ctx.magic, ctx.size);
        EXPECT_EQ(_mem_cmp(ctx.ptr, ctx.magic, ctx.size), 0);
        smem_free(ctx.ptr);
        EXPECT_EQ(max_block(heap), total_size);
    }
    /*
     * Apply for memory release sequentially and
     * test whether memory block merging is effective
     */
    {
        size_t i, max_free = 0;
        struct mem_test_context ctx[3];
        /* alloc mem */
        for (i = 0; i < sizeof(ctx) / sizeof(ctx[0]); i++)
        {
            ctx[i].magic = magic++;
            ctx[i].size = max_block(heap) / (sizeof(ctx) / sizeof(ctx[0]) - i);
            ctx[i].ptr = smem_alloc(heap, ctx[i].size);
            EXPECT_NE(ctx[i].ptr, nullptr);
            memset(ctx[i].ptr, ctx[i].magic, ctx[i].size);
        }
        /* All memory has been applied. The remaining memory should be 0 */
        EXPECT_EQ(max_block(heap), 0);
        /* Verify that the memory data is correct */
        for (i = 0; i < sizeof(ctx) / sizeof(ctx[0]); i++)
        {
            EXPECT_EQ(_mem_cmp(ctx[i].ptr, ctx[i].magic, ctx[i].size), 0);
        }
        /* Sequential memory release */
        for (i = 0; i < sizeof(ctx) / sizeof(ctx[0]); i++)
        {
            EXPECT_EQ(_mem_cmp(ctx[i].ptr, ctx[i].magic, ctx[i].size), 0);
            smem_free(ctx[i].ptr);
            max_free += ctx[i].size;
            EXPECT_EQ(max_block(heap) >= max_free, true);
        }
        /* Check whether the memory is fully merged */
        EXPECT_EQ(max_block(heap), total_size);
    }
    /*
     * Apply for memory release at an interval to
     * test whether memory block merging is effective
     */
    {
        size_t i, max_free = 0;
        struct mem_test_context ctx[3];
        /* alloc mem */
        for (i = 0; i < sizeof(ctx) / sizeof(ctx[0]); i++)
        {
            ctx[i].magic = magic++;
            ctx[i].size = max_block(heap) / (sizeof(ctx) / sizeof(ctx[0]) - i);
            ctx[i].ptr = smem_alloc(heap, ctx[i].size);
            EXPECT_NE(ctx[i].ptr, nullptr);
            memset(ctx[i].ptr, ctx[i].magic, ctx[i].size);
        }
        /* All memory has been applied. The remaining memory should be 0 */
        EXPECT_EQ(max_block(heap), 0);
        /* Verify that the memory data is correct */
        for (i = 0; i < sizeof(ctx) / sizeof(ctx[0]); i++)
        {
            EXPECT_EQ(_mem_cmp(ctx[i].ptr, ctx[i].magic, ctx[i].size), 0);
        }
        /* Release even address */
        for (i = 0; i < sizeof(ctx) / sizeof(ctx[0]); i++)
        {
            if (i % 2 == 0)
            {
                EXPECT_EQ(_mem_cmp(ctx[i].ptr, ctx[i].magic, ctx[i].size), 0);
                smem_free(ctx[i].ptr);
                EXPECT_EQ(max_block(heap) >= ctx[0].size, true);
            }
        }
        /* Release odd addresses and merge memory blocks */
        for (i = 0; i < sizeof(ctx) / sizeof(ctx[0]); i++)
        {
            if (i % 2 != 0)
            {
                EXPECT_EQ(_mem_cmp(ctx[i].ptr, ctx[i].magic, ctx[i].size), 0);
                smem_free(ctx[i].ptr);
                max_free += ctx[i - 1].size + ctx[i + 1].size;
                EXPECT_EQ(max_block(heap) >= max_free, true);
            }
        }
        /* Check whether the memory is fully merged */
        EXPECT_EQ(max_block(heap), total_size);
    }
    /* mem realloc test,Small - > Large */
    {
        /* Request a piece of memory for subsequent reallocation operations */
        struct mem_test_context ctx[3];
        ctx[0].magic = magic++;
        ctx[0].size = max_block(heap) / 3;
        ctx[0].ptr = smem_alloc(heap, ctx[0].size);
        EXPECT_NE(ctx[0].ptr, nullptr);
        memset(ctx[0].ptr, ctx[0].magic, ctx[0].size);
        /* Apply for a small piece of memory and split the continuous memory */
        ctx[1].magic = magic++;
        ctx[1].size = SMEM_ALIGN_SIZE;
        ctx[1].ptr = smem_alloc(heap, ctx[1].size);
        EXPECT_NE(ctx[1].ptr, nullptr);
        memset(ctx[1].ptr, ctx[1].magic, ctx[1].size);
        /* Check whether the maximum memory block is larger than the first piece of memory */
        EXPECT_EQ(max_block(heap) > ctx[0].size, true);
        /* Reallocate the first piece of memory */
        ctx[2].magic = magic++;
        ctx[2].size = max_block(heap);
        ctx[2].ptr = smem_realloc(heap, ctx[0].ptr, ctx[2].size);
        EXPECT_NE(ctx[2].ptr, nullptr);
        EXPECT_NE(ctx[0].ptr, ctx[2].ptr);
        EXPECT_EQ(_mem_cmp(ctx[2].ptr, ctx[0].magic, ctx[0].size), 0);
        memset(ctx[2].ptr, ctx[2].magic, ctx[2].size);
        /* Free the second piece of memory */
        EXPECT_EQ(_mem_cmp(ctx[1].ptr, ctx[1].magic, ctx[1].size), 0);
        smem_free(ctx[1].ptr);
        /* Free reallocated memory */
        EXPECT_EQ(_mem_cmp(ctx[2].ptr, ctx[2].magic, ctx[2].size), 0);
        smem_free(ctx[2].ptr);
        /* Check memory integrity */
        EXPECT_EQ(max_block(heap), total_size);
    }
    /* mem realloc test,Large - > Small */
    {
        size_t max_free;
        struct mem_test_context ctx;
        /* alloc a piece of memory */
        ctx.magic = magic++;
        ctx.size = max_block(heap) / 2;
        ctx.ptr = smem_alloc(heap, ctx.size);
        EXPECT_NE(ctx.ptr, nullptr);
        memset(ctx.ptr, ctx.magic, ctx.size);
        EXPECT_EQ(_mem_cmp(ctx.ptr, ctx.magic, ctx.size), 0);
        /* Get remaining memory */
        max_free = max_block(heap);
        /* Change memory size */
        ctx.size = ctx.size / 2;
        EXPECT_EQ((unsigned long)smem_realloc(heap, ctx.ptr, ctx.size), (unsigned long)ctx.ptr);
        /* Get remaining size */
        EXPECT_EQ(max_block(heap) > max_free, true);
        /* Free memory */
        EXPECT_EQ(_mem_cmp(ctx.ptr, ctx.magic, ctx.size), 0);
        smem_free(ctx.ptr);
        /* Check memory integrity */
        EXPECT_EQ(max_block(heap), total_size);
    }
    /* mem realloc test,equal */
    {
        size_t max_free;
        struct mem_test_context ctx;
        /* alloc a piece of memory */
        ctx.magic = magic++;
        ctx.size = max_block(heap) / 2;
        ctx.ptr = smem_alloc(heap, ctx.size);
        EXPECT_NE(ctx.ptr, nullptr);
        memset(ctx.ptr, ctx.magic, ctx.size);
        EXPECT_EQ(_mem_cmp(ctx.ptr, ctx.magic, ctx.size), 0);
        /* Get remaining memory */
        max_free = max_block(heap);
        /* Do not change memory size */
        EXPECT_EQ((unsigned long)smem_realloc(heap, ctx.ptr, ctx.size), (unsigned long)ctx.ptr);
        /* Get remaining size */
        EXPECT_EQ(max_block(heap) == max_free, true);
        /* Free memory */
        EXPECT_EQ(_mem_cmp(ctx.ptr, ctx.magic, ctx.size), 0);
        smem_free(ctx.ptr);
        /* Check memory integrity */
        EXPECT_EQ(max_block(heap), total_size);
    }
    /* release test resources */
    free(buf);
}

struct mem_alloc_context
{
    list_t node;
    size_t size;
    uint8_t magic;
};

struct mem_alloc_head
{
    list_t list;
    size_t count;
};

#define MEM_RANG_ALLOC_BLK_MIN  2
#define MEM_RANG_ALLOC_BLK_MAX  5
#define MEM_RANG_ALLOC_TEST_TIME 5

TEST_F(SmallMemTest, mem_alloc_test)
{
    struct mem_alloc_head head;
    uint8_t *buf;
    struct small_mem *heap;
    size_t total_size, size;
    struct mem_alloc_context *ctx;
    auto start_time = now();

    /* init */
    list_init(&head.list);
    head.count = 0;
    buf = (uint8_t *)malloc(TEST_MEM_SIZE);
    EXPECT_NE(buf, nullptr);
    EXPECT_EQ(SMEM_ALIGN((unsigned long)buf, SMEM_ALIGN_SIZE), (unsigned long)buf);
    memset(buf, 0xAA, TEST_MEM_SIZE);
    heap =  (struct small_mem *)smem_init(buf, TEST_MEM_SIZE);
    total_size = max_block(heap);
    EXPECT_NE(total_size, 0);
    /* test run */
    while (1)
    {
        auto curr_time = now();
        std::chrono::duration<double> elapsed = curr_time - start_time;
        if (elapsed.count() >= MEM_RANG_ALLOC_TEST_TIME)
        {
            break;
        }
        progress(elapsed.count() / MEM_RANG_ALLOC_TEST_TIME);

        /* %60 probability to perform alloc operation */
        if (rand() % 10 >= 4)
        {
            size = rand() % MEM_RANG_ALLOC_BLK_MAX + MEM_RANG_ALLOC_BLK_MIN;
            size *= sizeof(struct mem_alloc_context);
            ctx = (struct mem_alloc_context *)smem_alloc(heap, size);
            if (ctx == nullptr)
            {
                if (head.count == 0)
                {
                    break;
                }
                size = head.count / 2;
                while (size != head.count)
                {
                    ctx = list_first_entry(&head.list, struct mem_alloc_context, node);
                    list_remove(&ctx->node);
                    if (ctx->size > sizeof(*ctx))
                    {
                        if (_mem_cmp(&ctx[1], ctx->magic, ctx->size - sizeof(*ctx)) != 0)
                        {
                            EXPECT_EQ(0, true);
                        }
                    }
                    memset(ctx, 0xAA, ctx->size);
                    smem_free(ctx);
                    head.count --;
                }
                continue;
            }
            if (SMEM_ALIGN((unsigned long)ctx, SMEM_ALIGN_SIZE) != (unsigned long)ctx)
            {
                EXPECT_EQ(SMEM_ALIGN((unsigned long)ctx, SMEM_ALIGN_SIZE), (unsigned long)ctx);
            }
            memset(ctx, 0, size);
            list_init(&ctx->node);
            ctx->size = size;
            ctx->magic = rand() & 0xff;
            if (ctx->size > sizeof(*ctx))
            {
                memset(&ctx[1], ctx->magic, ctx->size - sizeof(*ctx));
            }
            list_insert_after(&head.list, &ctx->node);
            head.count += 1;
        }
        else
        {
            if (!list_isempty(&head.list))
            {
                ctx = list_first_entry(&head.list, struct mem_alloc_context, node);
                list_remove(&ctx->node);
                if (ctx->size > sizeof(*ctx))
                {
                    if (_mem_cmp(&ctx[1], ctx->magic, ctx->size - sizeof(*ctx)) != 0)
                    {
                        EXPECT_EQ(0, true);
                    }
                }
                memset(ctx, 0xAA, ctx->size);
                smem_free(ctx);
                head.count --;
            }
        }
    }
    while (!list_isempty(&head.list))
    {
        ctx = list_first_entry(&head.list, struct mem_alloc_context, node);
        list_remove(&ctx->node);
        if (ctx->size > sizeof(*ctx))
        {
            if (_mem_cmp(&ctx[1], ctx->magic, ctx->size - sizeof(*ctx)) != 0)
            {
                EXPECT_EQ(0, true);
            }
        }
        memset(ctx, 0xAA, ctx->size);
        smem_free(ctx);
        head.count --;
    }
    EXPECT_EQ(head.count, 0);
    EXPECT_EQ(max_block(heap), total_size);
    /* release test resources */
    free(buf);
}

#define MEM_RANG_REALLOC_BLK_MIN  0
#define MEM_RANG_REALLOC_BLK_MAX  5
#define MEM_RANG_REALLOC_TEST_TIME 5

struct mem_realloc_context
{
    size_t size;
    uint8_t magic;
};

struct mem_realloc_head
{
    struct mem_realloc_context **ctx_tab;
    size_t count;
};

TEST_F(SmallMemTest, mem_realloc_test)
{
    struct mem_realloc_head head;
    uint8_t *buf;
    struct small_mem *heap;
    size_t total_size, size, idx;
    struct mem_realloc_context *ctx;
    int res;
    auto start_time = now();

    size = SMEM_ALIGN(sizeof(struct mem_realloc_context), SMEM_ALIGN_SIZE) + SMEM_ALIGN_SIZE;
    size = TEST_MEM_SIZE / size;
    /* init */
    head.ctx_tab = nullptr;
    head.count = size;
    buf = (uint8_t *)malloc(TEST_MEM_SIZE);
    EXPECT_NE(buf, nullptr);
    EXPECT_EQ(SMEM_ALIGN((unsigned long)buf, SMEM_ALIGN_SIZE), (unsigned long)buf);
    memset(buf, 0xAA, TEST_MEM_SIZE);
    heap =  (struct small_mem *)smem_init(buf, TEST_MEM_SIZE);
    total_size = max_block(heap);
    EXPECT_NE(total_size, 0);
    /* init ctx tab */
    size = head.count * sizeof(struct mem_realloc_context *);
    head.ctx_tab = (struct mem_realloc_context **)smem_alloc(heap, size);
    EXPECT_NE(head.ctx_tab, nullptr);
    memset(head.ctx_tab, 0, size);
    /* test run */
    while (1)
    {
        auto curr_time = now();
        std::chrono::duration<double> elapsed = curr_time - start_time;
        if (elapsed.count() >= MEM_RANG_ALLOC_TEST_TIME)
        {
            break;
        }
        progress(elapsed.count() / MEM_RANG_ALLOC_TEST_TIME);

        size = rand() % MEM_RANG_ALLOC_BLK_MAX + MEM_RANG_ALLOC_BLK_MIN;
        size *= sizeof(struct mem_realloc_context);
        idx = rand() % head.count;
        ctx = (mem_realloc_context *)smem_realloc(heap, head.ctx_tab[idx], size);
        if (ctx == nullptr)
        {
            if (size == 0)
            {
                if (head.ctx_tab[idx])
                {
                    head.ctx_tab[idx] = nullptr;
                }
            }
            else
            {
                for (idx = 0; idx < head.count; idx++)
                {
                    ctx = head.ctx_tab[idx];
                    if (rand() % 2 && ctx)
                    {
                        if (ctx->size > sizeof(*ctx))
                        {
                            res = _mem_cmp(&ctx[1], ctx->magic, ctx->size - sizeof(*ctx));
                            if (res != 0)
                            {
                                EXPECT_EQ(res, 0);
                            }
                        }
                        memset(ctx, 0xAA, ctx->size);
                        smem_realloc(heap, ctx, 0);
                        head.ctx_tab[idx] = nullptr;
                    }
                }
            }
            continue;
        }
        /* check mem */
        if (head.ctx_tab[idx] != nullptr)
        {
            res = 0;
            if (ctx->size < size)
            {
                if (ctx->size > sizeof(*ctx))
                {
                    res = _mem_cmp(&ctx[1], ctx->magic, ctx->size - sizeof(*ctx));
                }
            }
            else
            {
                if (size > sizeof(*ctx))
                {
                    res = _mem_cmp(&ctx[1], ctx->magic, size - sizeof(*ctx));
                }
            }
            if (res != 0)
            {
                EXPECT_EQ(res, 0);
            }
        }
        /* init mem */
        ctx->magic = rand() & 0xff;
        ctx->size = size;
        if (ctx->size > sizeof(*ctx))
        {
            memset(&ctx[1], ctx->magic, ctx->size - sizeof(*ctx));
        }
        head.ctx_tab[idx] = ctx;
    }
    /* free all mem */
    for (idx = 0; idx < head.count; idx++)
    {
        ctx = head.ctx_tab[idx];
        if (ctx == nullptr)
        {
            continue;
        }
        if (ctx->size > sizeof(*ctx))
        {
            res = _mem_cmp(&ctx[1], ctx->magic, ctx->size - sizeof(*ctx));
            if (res != 0)
            {
                EXPECT_EQ(res, 0);
            }
        }
        memset(ctx, 0xAA, ctx->size);
        smem_realloc(heap, ctx, 0);
        head.ctx_tab[idx] = nullptr;
    }
    EXPECT_NE(max_block(heap), total_size);
    /* release test resources */
    free(buf);
}
