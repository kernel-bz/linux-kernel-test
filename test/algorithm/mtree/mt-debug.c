// SPDX-License-Identifier: GPL-2.0+
/*
 *	mt-debug.c: debug for maple-tree
 * 	Copyright (c) 2022 JaeJoon Jung <rgbi3307@gmail.com>
 */

#include "test/debug.h"

#include <linux/maple_tree.h>
#include <linux/xarray.h>

static const unsigned char mt_slots[] = {
    [maple_dense]		= MAPLE_NODE_SLOTS,
    [maple_leaf_64]		= MAPLE_RANGE64_SLOTS,
    [maple_range_64]	= MAPLE_RANGE64_SLOTS,
    [maple_arange_64]	= MAPLE_ARANGE64_SLOTS,
};

static const unsigned char mt_pivots[] = {
    [maple_dense]		= 0,
    [maple_leaf_64]		= MAPLE_RANGE64_SLOTS - 1,
    [maple_range_64]	= MAPLE_RANGE64_SLOTS - 1,
    [maple_arange_64]	= MAPLE_ARANGE64_SLOTS - 1,
};

//struct maple_big_node;
#define MAPLE_BIG_NODE_SLOTS	(MAPLE_RANGE64_SLOTS * 2 + 2)

struct maple_big_node {
    struct maple_pnode *parent;
    struct maple_enode *slot[MAPLE_BIG_NODE_SLOTS];
    unsigned long pivot[MAPLE_BIG_NODE_SLOTS - 1];
    unsigned long gap[MAPLE_BIG_NODE_SLOTS];
    unsigned long min;
    unsigned char b_end;
    enum maple_type type;
};

static void _mt_debug_node_info(struct maple_node *node)
{
    unsigned int offset;
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%20s: %lu\n", sizeof(*node));
    pr_view_on(stack_depth, "%20s: %lu\n", sizeof(node->mr64));
    pr_view_on(stack_depth, "%20s: %lu\n", sizeof(node->ma64));
    pr_view_on(stack_depth, "%20s: %lu\n", sizeof(node->alloc));

    pr_view_on(stack_depth, "%35s: %lu\n",
                   offset_of(struct maple_range_64, meta));
    pr_view_on(stack_depth, "%35s: %lu\n",
                   offset_of(struct maple_arange_64, meta));

    pr_fn_end_on(stack_depth);
}

static void _mt_debug_node_print_all(struct maple_node *node)
{
    unsigned int i;
    pr_fn_start_on(stack_depth);

    printf("\n\t==> ");
    printf("node->slot[%d]: ", MAPLE_NODE_SLOTS);
    for (i = 0; i < MAPLE_NODE_SLOTS; i++)
        printf("%p, ", node->slot[i]);

    printf("\n\t==> ");
    printf("node->mr64.pivot[%d]: ", MAPLE_RANGE64_SLOTS - 1);
    for (i = 0; i < MAPLE_RANGE64_SLOTS - 1; i++)
        printf("%lu, ", node->mr64.pivot[i]);

    printf("\n\t--> ");
    printf("node->mr64.slot[%d]: ", MAPLE_RANGE64_SLOTS);
    for (i = 0; i < MAPLE_RANGE64_SLOTS; i++)
        printf("%p, ",  node->mr64.slot[i]);

    printf("\n\t--> ");
    printf("node->mr64.pad[%d]: ", MAPLE_RANGE64_SLOTS - 1);
    for (i = 0; i < MAPLE_RANGE64_SLOTS - 1; i++)
        printf("%p, ", node->mr64.pad[i]);

    printf("\n\t==> ");
    printf("node->ma64.pivot[%d]: ", MAPLE_ARANGE64_SLOTS - 1);
    for (i = 0; i < MAPLE_ARANGE64_SLOTS - 1; i++)
        printf("%lu, ", node->ma64.pivot[i]);

    printf("\n\t--> ");
    printf("node->ma64.slot[%d]: ", MAPLE_ARANGE64_SLOTS);
    for (i = 0; i < MAPLE_ARANGE64_SLOTS; i++)
        printf("%p, ",  node->ma64.slot[i]);

    printf("\n\t--> ");
    printf("node->ma64.gap[%d]: ", MAPLE_ARANGE64_SLOTS);
    for (i = 0; i < MAPLE_ARANGE64_SLOTS; i++)
        printf("%lu, ", node->ma64.gap[i]);

    printf("\n\t==> ");
    printf("node->alloc.slot[%d]: ", MAPLE_ALLOC_SLOTS);
    for (i = 0; i < MAPLE_ALLOC_SLOTS; i++)
        printf("%p, ", node->alloc.slot[i]);

    printf("\n\n");
    pr_fn_end_on(stack_depth);
}

void mt_debug_big_node_print(struct maple_big_node *bnode)
{
    unsigned int i;

    pr_fn_start_on(stack_depth);
    pr_view_on(stack_depth, "%10s: %p\n", bnode->parent);
    pr_view_on(stack_depth, "%10s: %lu\n", bnode->min);
    pr_view_on(stack_depth, "%10s: %u\n", bnode->b_end);
    pr_view_on(stack_depth, "%10s: %d\n", bnode->type);

    printf("\n\t==> ");
    printf("bnode->pivot[%d]: ", MAPLE_BIG_NODE_SLOTS - 1);
    for (i = 0; i < MAPLE_BIG_NODE_SLOTS - 1; i++)
        printf("%lu, ", bnode->pivot[i]);

    printf("\n\t--> ");
    printf("bnode->slot[%d]: ", MAPLE_BIG_NODE_SLOTS);
    for (i = 0; i < MAPLE_BIG_NODE_SLOTS; i++)
        printf("%p, ", bnode->slot[i]);

    printf("\n\t--> ");
    printf("bnode->gap[%d]: ", MAPLE_BIG_NODE_SLOTS);
    for (i = 0; i < MAPLE_BIG_NODE_SLOTS; i++)
        printf("%lu, ", bnode->gap[i]);

    printf("\n\n");
}

//mas->node
void mt_debug_node_print(struct maple_tree *mt, struct maple_node *node)
{
    enum maple_type type = maple_leaf_64;
    void __rcu **slots;
    unsigned long *pivots, *gaps;
    unsigned char i, cnt, height;

    pr_fn_start_on(stack_depth);

    //_mt_debug_node_info(node);

    pr_view_on(stack_depth, "%20s : %p\n", mt);
    pr_view_on(stack_depth, "%20s : %p\n", mt->ma_flags);
    pr_view_on(stack_depth, "%20s : %p\n", mt->ma_root);
    pr_view_on(stack_depth, "%20s : %p\n", node);
    if ((unsigned long)node < 4096 || xa_is_err(node)) return;

    //mte_node_type
    type = ((unsigned long)node >> MAPLE_NODE_TYPE_SHIFT) &
        MAPLE_NODE_TYPE_MASK;

    //mte_to_node
    node = (struct maple_node *)((unsigned long)node & ~MAPLE_NODE_MASK);

    //pivots = ma_pivots(node, type);
    //slots = ma_slots(node, type);
    switch (type) {
    case maple_arange_64:
        pivots = node->ma64.pivot;
        slots = node->ma64.slot;
        gaps = node->ma64.gap;
        break;
    case maple_range_64:
    case maple_leaf_64:
        pivots = node->mr64.pivot;
        slots = node->mr64.slot;
        gaps = NULL;
        break;
    case maple_dense:
        pivots = NULL;
        slots = node->slot;
        gaps = NULL;
        break;
    default:
        pivots = NULL;
        slots = NULL;
        gaps = NULL;
    }

    //mt_height
    height = (mt->ma_flags & MT_FLAGS_HEIGHT_MASK) >> MT_FLAGS_HEIGHT_OFFSET;

    pr_view_on(stack_depth, "%20s : %p\n", node->parent);
    pr_view_on(stack_depth, "%20s : %d\n", height);
    pr_view_on(stack_depth, "%20s : %d\n", type);

    if (pivots || slots) {
        if (pivots) {
            cnt = mt_pivots[type];
            printf("\n\t==> node->pivot[%d]: ", cnt);
            for (i = 0; i < cnt; i++)
                //printf("%p, ", pivots[i]);
                printf("%lu, ", pivots[i]);
        }
        if (slots) {
            cnt = mt_slots[type];
            printf("\n\t==> node->slot[%d]: ", cnt);
            for (i = 0; i < cnt; i++)
                printf("%p, ", slots[i]);
        }
        if (gaps) {
            cnt = mt_slots[type];
            printf("\n\t==> node->gap[%d]: ", cnt);
            for (i = 0; i < cnt; i++)
                printf("%lu, ", gaps[i]);
        }

    } else {
        _mt_debug_node_print_all(node);
    }
    printf("\n\n");
    pr_fn_end_on(stack_depth);
}

void mt_debug_mas_print(struct ma_state *mas)
{
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%20s : %lu\n", mas->index);
    pr_view_on(stack_depth, "%20s : %lu\n", mas->last);
    pr_view_on(stack_depth, "%20s : %p\n", mas->node);
    pr_view_on(stack_depth, "%20s : %lu\n", mas->min);
    pr_view_on(stack_depth, "%20s : %lu\n", mas->max);
    pr_view_on(stack_depth, "%20s : %p\n", mas->alloc);
    pr_view_on(stack_depth, "%20s : %d\n", mas->depth);
    pr_view_on(stack_depth, "%20s : %d\n", mas->offset);
    pr_view_on(stack_depth, "%20s : 0x%X\n", mas->mas_flags);

    pr_fn_end_on(stack_depth);
}

void mt_debug_wr_mas_print(struct ma_wr_state *wr_mas)
{
    pr_fn_start_on(stack_depth);

    mt_debug_mas_print(wr_mas->mas);

    pr_view_on(stack_depth, "%20s : %p\n", wr_mas->node);
    pr_view_on(stack_depth, "%20s : %d\n", wr_mas->type);
    pr_view_on(stack_depth, "%20s : %lu\n", wr_mas->r_min);
    pr_view_on(stack_depth, "%20s : %lu\n", wr_mas->r_max);
    pr_view_on(stack_depth, "%20s : %d\n", wr_mas->offset_end);
    pr_view_on(stack_depth, "%20s : %d\n", wr_mas->node_end);
    pr_view_on(stack_depth, "%20s : %lu\n", wr_mas->end_piv);
    pr_view_on(stack_depth, "%20s : %p\n", wr_mas->entry);
    pr_view_on(stack_depth, "%20s : %p\n", wr_mas->content);

    pr_fn_end_on(stack_depth);
}

void mt_debug_mas_alloc_print(struct ma_state *mas)
{
    pr_fn_start_on(stack_depth);

    mt_debug_mas_print(mas);

    pr_view_on(stack_depth, "%30s : %lu\n", mas->alloc->total);
    pr_view_on(stack_depth, "%30s : %u\n", mas->alloc->node_count);
    pr_view_on(stack_depth, "%30s : %u\n", mas->alloc->request_count);

    pr_fn_end_on(stack_depth);
}
