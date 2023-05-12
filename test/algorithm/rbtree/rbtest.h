// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/algorithm/rbtree/rbtest.h
 *  Red-Black Tree Test Common Header
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stdio.h>
#include <stdlib.h>
#include <linux/rbtree.h>

#include "test/user-common.h"
#include "test/debug.h"

struct rb_test_struct {
    u32 key;
    struct rb_node rb;

    ///following fields used for testing augmented rbtree functionality
    u32 val;
    u32 augmented;
};


void rb_test_insert(struct rb_test_struct *rbs, struct rb_root *root);
void rb_test_init(int cnt, struct rb_test_struct *node, struct rb_root *root);
void rb_test_erase_all(struct rb_root *rb_test_root);
struct rb_test_struct *rb_test_search(struct rb_root *root, int value);
void rb_test_output(struct rb_root *root);
void rb_test_output_postorder(struct rb_root *root);

//lib/rbtree_test.c
int rbtree_test_all(void);
