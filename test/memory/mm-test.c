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
#include <mm/slab.h>

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

    slab_state_set(DOWN);

    memblock_dump_all();

#ifdef MM_MEMBLOCK_DEBUG
    memblock_add(2000, 200);
    memblock_add(4000, 400);
    memblock_add(8000, 800);
    memblock_add(6000, 600);
    memblock_add(5000, 1000);
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

    memblock_dump_all();

    pr_fn_end_on(stack_depth);
}
