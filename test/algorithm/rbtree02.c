// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/algorithm/rbtree02.c
 *  Red-Black Tree Test 02
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
//#include <sys/time.h>

#include <linux/time.h>
#include <linux/rbtree_augmented.h>
#include <linux/random_.h>

#include "test/algorithm/rbtree/rbtest.h"

#define NodeCnt     8
static struct rb_test_struct Nodes[NodeCnt];

static inline void rbtree_erase(struct rb_test_struct *node, struct rb_root *root)
{
	rb_erase(&node->rb, root);
}

static inline u32 augment_recompute(struct rb_test_struct *node)
{
	u32 max = node->val, child_augmented;
	if (node->rb.rb_left) {
        child_augmented = rb_entry(node->rb.rb_left, struct rb_test_struct,
					   rb)->augmented;
		if (max < child_augmented)
			max = child_augmented;
	}
	if (node->rb.rb_right) {
        child_augmented = rb_entry(node->rb.rb_right, struct rb_test_struct,
					   rb)->augmented;
		if (max < child_augmented)
			max = child_augmented;
	}
	return max;
}

RB_DECLARE_CALLBACKS_MAX(static, augment_callbacks, struct rb_test_struct, rb,
		     u32, augmented, augment_recompute)

static void rbtree_insert_augmented(struct rb_test_struct *node, struct rb_root *root)
{
	struct rb_node **new = &root->rb_node, *rb_parent = NULL;
	u32 key = node->key;
	u32 val = node->val;
    struct rb_test_struct *parent;

	while (*new) {
		rb_parent = *new;
        parent = rb_entry(rb_parent, struct rb_test_struct, rb);
		if (parent->augmented < val)
			parent->augmented = val;
		if (key < parent->key)
			new = &parent->rb.rb_left;
		else
			new = &parent->rb.rb_right;
	}

	node->augmented = val;
	rb_link_node(&node->rb, rb_parent, new);
    rb_insert_augmented(&node->rb, root, &augment_callbacks);
}

static void rbtree_erase_augmented(struct rb_test_struct *node, struct rb_root *root)
{
	rb_erase_augmented(&node->rb, root, &augment_callbacks);
}

static bool is_red(struct rb_node *rb)
{
	return !(rb->__rb_parent_color & 1);
}

static int black_path_count(struct rb_node *rb)
{
	int count;
	for (count = 0; rb; rb = rb_parent(rb))
		count += !is_red(rb);
	return count;
}

static void rbtree_nodes_init(void)
{
    int keys[2][8] = { {  10,  20,  30,  40,  50,  60,  70,  80 },
                       { 100, 200, 300, 400, 500, 600, 700, 800 } };
	int i;
	for (i = 0; i < NodeCnt; i++) {
        Nodes[i].key = keys[0][i];
        Nodes[i].val = keys[1][i];
	}
}

static void rbtree_nodes_travel(struct rb_root *root)
{
    int keys[8] = { 10,  20,  30,  40,  50,  60,  70,  80 };
    int i;
    for (i = 0; i < NodeCnt; i++) {
        rb_test_search(root, keys[i]);
    }
}

static void rbtree_augmented_test(struct rb_root *root)
{
    int i;

    for (i = 0; i < NodeCnt; i++)
        rbtree_insert_augmented(Nodes + i, root);

    rbtree_nodes_travel(root);

    rb_test_output(root);

    rb_test_output_postorder(root);

    for (i = 0; i < NodeCnt; i++)
        rbtree_erase_augmented(Nodes + i, root);

}

void rbtree_test02(void)
{
    struct rb_root rb_test_root = RB_ROOT;

    pr_fn_start(stack_depth);

    rbtree_nodes_init();

    rbtree_augmented_test(&rb_test_root);

    pr_fn_end(stack_depth);
}

