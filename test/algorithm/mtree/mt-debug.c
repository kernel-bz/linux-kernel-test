// SPDX-License-Identifier: GPL-2.0+
/*
 *	mt-debug.c: debug for maple-tree
 * 	Copyright (c) 2022 JaeJoon Jung <rgbi3307@gmail.com>
 */

#include "test/debug.h"

#include <linux/maple_tree.h>

void mt_debug_node_print(struct maple_tree *mt, struct maple_node *node)
{
    pr_fn_start_enable(stack_depth);
    unsigned int i;

    //pr_view_enable(stack_depth, "%20s : %ld\n", sizeof(*node));
    //pr_view_enable(stack_depth, "%20s : %ld\n", sizeof(node->mr64));
    //pr_view_enable(stack_depth, "%20s : %ld\n", sizeof(node->ma64));
    //pr_view_enable(stack_depth, "%20s : %ld\n", sizeof(node->alloc));

    pr_view_enable(stack_depth, "%20s : %p\n", mt);
    pr_view_enable(stack_depth, "%20s : %p\n", mt->ma_flags);
    pr_view_enable(stack_depth, "%20s : %p\n", node);
    pr_view_enable(stack_depth, "%20s : %p\n", node->parent);
    pr_view_enable(stack_depth, "%20s : %p\n", node->mr64.parent);
    pr_view_enable(stack_depth, "%20s : %p\n", node->ma64.parent);
    pr_view_enable(stack_depth, "%20s : %p\n", node->pad);
    pr_view_enable(stack_depth, "%20s : %p\n", node->alloc.total);

    printf("\n\t==> ");
    printf("node->slot[%d]: ", MAPLE_NODE_SLOTS);
    for (i = 0; i < MAPLE_NODE_SLOTS; i++)
        printf("%p, ", node->slot[i]);

    printf("\n\t==> ");
    printf("node->mr64.pivot[%d]: ", MAPLE_RANGE64_SLOTS - 1);
    for (i = 0; i < MAPLE_RANGE64_SLOTS - 1; i++)
        printf("%p, ", node->mr64.pivot[i]);

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
        printf("%p, ", node->ma64.pivot[i]);

    printf("\n\t--> ");
    printf("node->ma64.slot[%d]: ", MAPLE_ARANGE64_SLOTS);
    for (i = 0; i < MAPLE_ARANGE64_SLOTS; i++)
        printf("%p, ",  node->ma64.slot[i]);

    printf("\n\t--> ");
    printf("node->ma64.gap[%d]: ", MAPLE_ARANGE64_SLOTS);
    for (i = 0; i < MAPLE_ARANGE64_SLOTS; i++)
        printf("%p, ", node->ma64.gap[i]);

    printf("\n\t==> ");
    printf("node->alloc.slot[%d]: ", MAPLE_ALLOC_SLOTS);
    for (i = 0; i < MAPLE_ALLOC_SLOTS; i++)
        printf("%p, ", node->alloc.slot[i]);

    printf("\n\n");
    pr_fn_end_enable(stack_depth);
}

void mt_debug_mas_print(struct ma_state *mas)
{
    pr_fn_start_enable(stack_depth);

    pr_view_enable(stack_depth, "%20s : %lu\n", mas->index);
    pr_view_enable(stack_depth, "%20s : %lu\n", mas->last);
    pr_view_enable(stack_depth, "%20s : %p\n", mas->node);
    pr_view_enable(stack_depth, "%20s : %lu\n", mas->min);
    pr_view_enable(stack_depth, "%20s : %lu\n", mas->max);
    pr_view_enable(stack_depth, "%20s : %p\n", mas->alloc);
    pr_view_enable(stack_depth, "%20s : %d\n", mas->depth);
    pr_view_enable(stack_depth, "%20s : %d\n", mas->offset);
    pr_view_enable(stack_depth, "%20s : 0x%X\n", mas->mas_flags);

    pr_fn_end_enable(stack_depth);
}

void mt_debug_wr_mas_print(struct ma_wr_state *wr_mas)
{
    pr_fn_start_enable(stack_depth);

    mt_debug_mas_print(wr_mas->mas);

    pr_view_enable(stack_depth, "%20s : %d\n", wr_mas->type);
    pr_view_enable(stack_depth, "%20s : %p\n", wr_mas->node);
    pr_view_enable(stack_depth, "%20s : %lu\n", wr_mas->r_min);
    pr_view_enable(stack_depth, "%20s : %lu\n", wr_mas->r_max);
    pr_view_enable(stack_depth, "%20s : %d\n", wr_mas->offset_end);
    pr_view_enable(stack_depth, "%20s : %d\n", wr_mas->node_end);
    pr_view_enable(stack_depth, "%20s : %p\n", wr_mas->entry);
    pr_view_enable(stack_depth, "%20s : %p\n", wr_mas->content);

    pr_fn_end_enable(stack_depth);
}

void mt_debug_mas_alloc_print(struct ma_state *mas)
{
    pr_fn_start_enable(stack_depth);

    mt_debug_mas_print(mas);

    pr_view_enable(stack_depth, "%30s : %lu\n", mas->alloc->total);
    pr_view_enable(stack_depth, "%30s : %u\n", mas->alloc->node_count);
    pr_view_enable(stack_depth, "%30s : %u\n", mas->alloc->request_count);

    pr_fn_end_enable(stack_depth);
}
