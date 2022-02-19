// SPDX-License-Identifier: GPL-2.0+
/*
 * mt-test.c: Userspace test for maple tree test-suite
 * Copyright (c) 2022 JaeJoon Jung <rgbi3307@gmail.com> on the www.kernel.bz
 */

#include "test/debug.h"
#include "test/mt-debug.h"
#include "./test.h"

static DEFINE_MTREE(mtree);

void mtree_basic_store_test(void)
{
    unsigned long i;
    int first, last, step;

    __fpurge(stdin);
    printf("Input first index for testing: ");
    scanf("%d", &first);
    printf("Input last index for testing: ");
    scanf("%d", &last);
    printf("Input step size for testing: ");
    scanf("%d", &step);

    maple_tree_init();

    for (i = first; i <= last; i += step) {
        mtree_store(&mtree, i, xa_mk_value(i), GFP_KERNEL);
        if (!(i % 8))
            mt_validate(&mtree);
    }
    mt_validate(&mtree);

    void *entry;
    unsigned long key = 7;
    MA_STATE(mas, &mtree, key, key);
    //mas_set(&mas, key);
    entry = mas_walk(&mas);

    //mas_set_range(&mas, key, key + 8);
    //entry = mas_walk(&mas);

    mtree_destroy(&mtree);
}

void mtree_basic_walk_test(void)
{
    maple_tree_init();

    MA_STATE(mas, &mtree, 0, 0);
    void *ptr;

    //ptr = &mtree_basic_walk_test;
    ptr = xa_mk_value(0);

    mas_lock(&mas);

    mas_set(&mas, 0);
    mas_store_gfp(&mas, ptr, GFP_KERNEL);

    mas_set(&mas, 0);
    ptr = mas_walk(&mas);
    MT_BUG_ON(&mtree, ptr == NULL);

    mas_set(&mas, 1);
    mas_store_gfp(&mas, ptr, GFP_KERNEL);

    mas_set(&mas, 1);
    ptr = mas_walk(&mas);
    //MT_BUG_ON(&mtree, ptr != &mtree_basic_walk_test);
    MT_BUG_ON(&mtree, ptr != xa_mk_value(0));

    mas_set(&mas, 2);
    ptr = mas_walk(&mas);
    MT_BUG_ON(&mtree, ptr != NULL);

    mas_unlock(&mas);
    mtree_destroy(&mtree);
}
