// SPDX-License-Identifier: GPL-2.0+
/*
 * mt-test.c: Userspace test for maple tree test-suite
 * Copyright (c) 2022 JaeJoon Jung <rgbi3307@gmail.com> on the www.kernel.bz
 */

#include "test/debug.h"
#include "test/mt-debug.h"
#include "./test.h"

static DEFINE_MTREE(mtree);

void mtree_info(void)
{
    struct maple_node node;
    unsigned int offset;
    pr_fn_start_enable(stack_depth);

    pr_view_enable(stack_depth, "%20s: %lu\n", sizeof(node));
    pr_view_enable(stack_depth, "%20s: %lu\n", sizeof(node.mr64));
    pr_view_enable(stack_depth, "%20s: %lu\n", sizeof(node.ma64));
    pr_view_enable(stack_depth, "%20s: %lu\n", sizeof(node.alloc));

    pr_view_enable(stack_depth, "%40s: %lu\n",
                   offset_of(struct maple_range_64, meta));
    pr_view_enable(stack_depth, "%40s: %lu\n",
                   offset_of(struct maple_arange_64, meta));

    pr_view(stack_depth, "%30s: %u\n", MAPLE_NODE_SLOTS);
    pr_view(stack_depth, "%30s: %u\n", MAPLE_RANGE64_SLOTS);
    pr_view(stack_depth, "%30s: %u\n", MAPLE_ARANGE64_SLOTS);
    pr_view(stack_depth, "%30s: %u\n", MAPLE_ARANGE64_META_MAX);
    pr_view(stack_depth, "%30s: %u\n", MAPLE_ALLOC_SLOTS);

    pr_fn_end_enable(stack_depth);
}

void mtree_basic_store_test(void)
{
    maple_tree_init();

    //mas_store_root
    mtree_store(&mtree, 0, xa_mk_value(0), GFP_KERNEL);
    //mas_root_expand
    mtree_store(&mtree, 1, xa_mk_value(1), GFP_KERNEL);

    //mas_wr_append
    mtree_store(&mtree, 2, xa_mk_value(2), GFP_KERNEL);
    mtree_store(&mtree, 3, xa_mk_value(3), GFP_KERNEL);
    //mas_wr_modify(Direct Replacement)
    mtree_store(&mtree, 2, xa_mk_value(2), GFP_KERNEL);

    //mas_wr_node_store
    mtree_store(&mtree, 6, xa_mk_value(6), GFP_KERNEL);
    mt_validate(&mtree);

    //mas_wr_node_store
    mtree_store(&mtree, 15, xa_mk_value(15), GFP_KERNEL);
    mt_validate(&mtree);

    //mas_wr_node_store
    mtree_store(&mtree, 4, xa_mk_value(4), GFP_KERNEL);
    mt_validate(&mtree);

    mtree_erase(&mtree, 4);
    mt_validate(&mtree);

    mtree_destroy(&mtree);
}

void mtree_basic_store_range_test(void)
{
    maple_tree_init();

    mtree_store_range(&mtree, 0, 4, xa_mk_value(0), GFP_KERNEL);
    mt_validate(&mtree);

    mtree_store_range(&mtree, 8, 12, xa_mk_value(8), GFP_KERNEL);
    mt_validate(&mtree);

    mtree_store(&mtree, 7, xa_mk_value(7), GFP_KERNEL);
    mt_validate(&mtree);

    mtree_store(&mtree, 10, xa_mk_value(10), GFP_KERNEL);
    mt_validate(&mtree);

    void *entry;
    MA_STATE(mas, &mtree, 6, 9);
    entry = mas_walk(&mas);

    mas_set_range(&mas, 9, 14);
    entry = mas_walk(&mas);

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

void mtree_store_loop_test(void)
{
    unsigned long i;
    int first, last, step, type;

    __fpurge(stdin);
    printf("Input first index for testing: ");
    scanf("%d", &first);
    printf("Input last index for testing: ");
    scanf("%d", &last);
    printf("Input step size for testing: ");
    scanf("%d", &step);
    printf("Input maple tree type[2:Range, 3:Arange]: ");
    scanf("%d", &type);

    if (type == 3)
        mt_init_flags(&mtree, MT_FLAGS_ALLOC_RANGE);

    maple_tree_init();

    for (i = first; i <= last; i += step) {
        mtree_store(&mtree, i, xa_mk_value(i), GFP_KERNEL);
        mt_validate(&mtree);
    }
    mt_validate(&mtree);

    /*
    void *entry;
    unsigned long key = 8;
    MA_STATE(mas, &mtree, key, key);
    entry = mas_walk(&mas);
    */

    mtree_destroy(&mtree);
}

/**
void *mtree_load(struct maple_tree *mt, unsigned long index);
int mtree_insert(struct maple_tree *mt, unsigned long index,
        void *entry, gfp_t gfp);
int mtree_insert_range(struct maple_tree *mt, unsigned long first,
        unsigned long last, void *entry, gfp_t gfp);
int mtree_store_range(struct maple_tree *mt, unsigned long first,
              unsigned long last, void *entry, gfp_t gfp);
int mtree_store(struct maple_tree *mt, unsigned long index,
        void *entry, gfp_t gfp);
void *mtree_erase(struct maple_tree *mt, unsigned long index);
void mtree_destroy(struct maple_tree *mt);
void __mt_destroy(struct maple_tree *mt);
*/
void mtree_store_load_erase_test(void)
{
    unsigned long index = 0;
    void *content;
    int first, last, step;

    __fpurge(stdin);
    printf("Input first index for testing: ");
    scanf("%d", &first);
    printf("Input last index for testing: ");
    scanf("%d", &last);
    printf("Input step size for testing: ");
    scanf("%d", &step);

    pr_fn_start_enable(stack_depth);

    maple_tree_init();

    for (index = first; index < last; index += step) {
        mtree_store(&mtree, index, xa_mk_value(index), GFP_KERNEL);
        //mtree_insert(&mtree, index, xa_mk_value(index), GFP_KERNEL);
    }
    mt_validate(&mtree);

    index = 4;
    content = mtree_load(&mtree, index);
    pr_view_enable(stack_depth, "%10s: %p\n", content);
    content = mtree_erase(&mtree, index);
    pr_view_enable(stack_depth, "%10s: %p\n", content);

    mt_validate(&mtree);

    for (index = first; index < last; index += step) {
        mtree_erase(&mtree, index);
        mt_validate(&mtree);
    }
    mt_validate(&mtree);

    mtree_destroy(&mtree);
    pr_fn_end_enable(stack_depth);
}
