// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/memory/mm-test.c
 *  Memory Test Module
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stdio.h>
#include <stdlib.h>

#include "test/config.h"
#include "test/debug.h"

#include <linux/memblock.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/err.h>

#define MM_MEMBLOCK_DEBUG

int mm_memblock_add(unsigned int size)
{
    pr_fn_start_on(stack_depth);
    void *p;

    p = kzalloc_node(200, GFP_NOWAIT, -1);
    if (!p) {
        pr_err("error!\n");
        return -1;
    }
    memblock_add(virt_to_page(p), size);

    pr_fn_end_on(stack_depth);
    return size;
}

void mm_memblock_test(void)
{
    pr_fn_start_on(stack_depth);

    //slab_state_set(DOWN);

    memblock_dump_all();

#ifdef MM_MEMBLOCK_DEBUG
    memblock_add(2000, 800);
    memblock_add(4000, 400);
    memblock_add(8000, 700);
    memblock_add(6000, 400);
    //memblock_add(5000, 600);
    memblock_add(5000, 1000);
    //memblock_add(5000, 3000);
    memblock_add(9000, 600);
    /*
    memblock_reserve(2200, 220);
    memblock_reserve(4400, 440);
    memblock_reserve(8800, 880);
    memblock_reserve(6600, 660);
    memblock_reserve(5500, 1100);
    */
#else
    mm_memblock_add(200);
    mm_memblock_add(400);
    mm_memblock_add(800);
    mm_memblock_add(600);
    mm_memblock_add(1000);
#endif

    memblock_alloc(600, SMP_CACHE_BYTES);
    memblock_alloc(600, SMP_CACHE_BYTES);
    memblock_alloc(600, SMP_CACHE_BYTES);
    memblock_alloc(600, SMP_CACHE_BYTES);
    memblock_alloc(600, SMP_CACHE_BYTES);

    memblock_dump_all();

    pr_fn_end_on(stack_depth);
}

void mm_constant_infos(void)
{
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%20s : %u\n", PAGE_SHIFT);
    pr_view_on(stack_depth, "%20s : %u\n", PAGE_SIZE);
    pr_view_on(stack_depth, "%20s : %p\n", PAGE_MASK);
    pr_view_on(stack_depth, "%20s : %p\n", ~PAGE_MASK);
    pr_view_on(stack_depth, "%20s : %u\n", PAGE_ALIGN(2000));
    pr_view_on(stack_depth, "%20s : %u\n", PAGE_ALIGN(4000));
    pr_view_on(stack_depth, "%20s : %u\n", PAGE_ALIGN(6000));
    pr_view_on(stack_depth, "%20s : %u\n", PAGE_ALIGN(8000));
    pr_view_on(stack_depth, "%20s : %u\n", PAGE_ALIGN(9000));
    pr_view_on(stack_depth, "%30s : %u\n", DEFAULT_MAX_MAP_COUNT);
    pr_view_on(stack_depth, "%30s : %u\n", MAX_ERRNO);
    pr_view_on(stack_depth, "%30s : %p\n", -MAX_ERRNO);
    pr_view_on(stack_depth, "%30s : %p\n", (unsigned long)-MAX_ERRNO);
    //#define IS_ERR_VALUE(x) unlikely((unsigned long)(void *)(x) >= (unsigned long)-MAX_ERRNO)

    pr_view_on(stack_depth, "%30s : %u\n", MAX_ORDER);
    pr_view_on(stack_depth, "%30s : %u\n", KMALLOC_MAX_CACHE_SIZE);
    pr_view_on(stack_depth, "%30s : %u\n", KMALLOC_MIN_SIZE);
    pr_view_on(stack_depth, "%30s : %u\n", KMALLOC_SHIFT_HIGH);
    pr_view_on(stack_depth, "%30s : %u\n", KMALLOC_SHIFT_LOW);
    pr_view_on(stack_depth, "%30s : %u\n", kmalloc_index(8));
    pr_view_on(stack_depth, "%30s : %u\n", kmalloc_index(16));
    pr_view_on(stack_depth, "%30s : %u\n", kmalloc_index(32));
    pr_view_on(stack_depth, "%30s : %u\n", kmalloc_index(64));
    pr_view_on(stack_depth, "%30s : %u\n", kmalloc_index(128));
    unsigned int size;
    pr_view_on(stack_depth, "%30s : %u\n", sizeof(void *));
    for (size = 0; size < 128; size += 13)
        pr_view_on(stack_depth, "%30s : %u\n", ALIGN(size, sizeof(void *)));

    pr_fn_end_on(stack_depth);
}
