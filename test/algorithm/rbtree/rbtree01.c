// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/algorithm/rbtree01.c
 *  Red-Black Tree Test 01
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stdio.h>
#include <stdlib.h>
#include <linux/rbtree.h>

#include "test/user-common.h"
#include "test/algorithm/rbtree/rbtest.h"

void rbtree_test01(void)
{
    struct rb_root rb_test_root = RB_ROOT;
    struct rb_test_struct *rbs;
	struct rb_node *node;
    u32 nr, key;

    pr_fn_start(stack_depth);

    __fpurge(stdin);
    printf("Input Test Counter Number: ");
    scanf("%u", &nr);

	//insert to rbtree
    rb_test_init(nr, rbs, &rb_test_root);

    rb_test_output(&rb_test_root);

    key = 4;
    rbs = rb_test_search(&rb_test_root, key);
    if (rbs) {
        pr_view(stack_depth, "%5s : rb_erase(): %d\n", key);
        rb_erase(&rbs->rb, &rb_test_root);
        //free(rbs);
    }

    rb_test_output(&rb_test_root);

    rb_test_erase_all(&rb_test_root);

    rb_test_output(&rb_test_root);

    pr_fn_end(stack_depth);
}
