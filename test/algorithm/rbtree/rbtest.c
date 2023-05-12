// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/algorithm/rbtree/rbtest.c
 *  Red-Black Tree Test Common Source
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stdio.h>
#include <stdlib.h>
#include <linux/rbtree.h>

#include "test/debug.h"
#include "test/user-common.h"
#include "test/algorithm/rbtree/rbtest.h"

void rb_test_insert(struct rb_test_struct *node, struct rb_root *root)
{
    struct rb_node **new = &root->rb_node, *parent = NULL;
    u32 key = node->key;
    //unsigned int level = 0;

    while (*new) {
        parent = *new;
        //level++;
        ///container_of(ptr, type, member)
        if (key < rb_entry(parent, struct rb_test_struct, rb)->key)
            new = &parent->rb_left;
        else
            new = &parent->rb_right;
    }

    rb_link_node(&node->rb, parent, new);
    rb_insert_color(&node->rb, root);
}

void rb_test_init(int cnt, struct rb_test_struct *node, struct rb_root *root)
{
    int i;
    for (i = 0; i < cnt; i++) {
        node = malloc(sizeof(*node));
        node->key = i;
        node->val = i;
        node->augmented = i;
        rb_test_insert(node, root);
    }
}

void rb_test_erase_all(struct rb_root *rb_test_root)
{
    struct rb_node *rb;

    pr_fn_start(stack_depth);

    for (rb = rb_first(rb_test_root); rb; rb = rb_next(rb))
        rb_erase(rb, rb_test_root);

    pr_fn_end(stack_depth);
}

struct rb_test_struct *rb_test_search(struct rb_root *root, int key)
{
    struct rb_node *rb = root->rb_node;  //top of the tree
    struct rb_test_struct *rb_test;

    pr_fn_start(stack_depth);
    pr_out(stack_depth, "key: %d\n", key);
    pr_out(stack_depth, "root, ");

    while (rb) {
        rb_test = rb_entry(rb, struct rb_test_struct, rb);

        if (rb_test->key > key) {
            rb = rb->rb_left;
            printf("left, ");
        } else if (rb_test->key < key) {
            rb = rb->rb_right;
            printf("right, ");
        } else
            goto _found;
    }
    printf("NOT found.\n");
    return NULL;

_found:
    printf("found(%p).\n", rb_test);
    pr_fn_end(stack_depth);

    return rb_test;
}

void rb_test_output(struct rb_root *root)
{
    struct rb_node *rb;
    struct rb_test_struct *rb_test;

    pr_fn_start(stack_depth);

    for (rb = rb_first(root); rb; rb = rb_next(rb)) {
        rb_test = rb_entry(rb, struct rb_test_struct, rb);
        pr_out(stack_depth, "key, val, aug: %u, %u, %u\n",
               rb_test->key, rb_test->val, rb_test->augmented);
    }

    pr_fn_end(stack_depth);
}

void rb_test_output_postorder(struct rb_root *root)
{
    struct rb_node *rb;
    struct rb_test_struct *rb_test;

    pr_fn_start(stack_depth);

    for (rb = rb_first_postorder(&root->rb_node); rb; rb = rb_next_postorder(rb)) {
        rb_test = rb_entry(rb, struct rb_test_struct, rb);
        pr_out(stack_depth, "key, val, aug: %u, %u, %u\n",
               rb_test->key, rb_test->val, rb_test->augmented);
    }

    pr_fn_end(stack_depth);
}

