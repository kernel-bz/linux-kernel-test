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

struct rb_test_struct {
	int value;
	struct rb_node node;
};

static struct rb_test_struct *rb_test_search(struct rb_root *root, int value)
{
	struct rb_node *node = root->rb_node;  //top of the tree

	while (node) {
        struct rb_test_struct *this = rb_entry(node, struct rb_test_struct, node);	//container_of

	    if (this->value > value)
			node = node->rb_left;
	    else if (this->value < value)
			node = node->rb_right;
	    else
			return this;  //found it
  	}
	return NULL;
}

static void rb_test_insert(struct rb_root *root, struct rb_test_struct *rbs)
{
	struct rb_node **link = &root->rb_node, *parent = NULL;
    int value = rbs->value;

	//go to the bottom of the tree
	while (*link) {
	    parent = *link;
        struct rb_test_struct *this = rb_entry(parent, struct rb_test_struct, node);

	    if (this->value > value)
			link = &(*link)->rb_left;
	    else
			link = &(*link)->rb_right;
	}

    //put the rbs node there
    rb_link_node(&(rbs->node), parent, link);
    rb_insert_color(&(rbs->node), root);
}

static void rb_test_output(struct rb_root *rb_test_root)
{
	struct rb_node *node;

    printf("\t output rb_node: ");
    for (node = rb_first(rb_test_root); node; node = rb_next(node))
        printf("%d, ", rb_entry(node, struct rb_test_struct, node)->value);
	printf("\n");
}

static void rb_test_drop(struct rb_root *rb_test_root)
{
	struct rb_node *node;

    printf("\t drop rb_node: ");
    for (node = rb_first(rb_test_root); node; node = rb_next(node))
        rb_erase(node, rb_test_root);

    printf("rb_test_root->rb_node: %p\n", rb_test_root->rb_node);
    //rb_test_root->rb_node = NULL;
}

void rbtree_test01(void)
{
    struct rb_root rb_test_root = RB_ROOT;
    struct rb_test_struct *rbs;
	struct rb_node *node;
	int i;

	//insert to rbtree
	for (i = 0; i < 20; i++) {
        rbs = malloc(sizeof(struct rb_test_struct));
        rbs->value = i%8;
        rb_test_insert(&rb_test_root, rbs);
	}
	//output from rbtree
    rb_test_output(&rb_test_root);

    rbs = rb_test_search(&rb_test_root, 4);
    if (rbs) {
        printf("\t searched value=%d\n", rbs->value);
        printf("\t erasing value=%d\n", rbs->value);
        rb_erase(&rbs->node, &rb_test_root);
        //free(rbs);
    }
    else printf ("\t Cannot find value: %d\n", rbs->value);

    rb_test_output(&rb_test_root);

    rb_test_drop(&rb_test_root);
    rb_test_output(&rb_test_root);

	//insert to rbtree
	for (i = 0; i < 20; i++) {
        rbs = malloc(sizeof(struct rb_test_struct));
        rbs->value = i;
        rb_test_insert(&rb_test_root, rbs);
	}
	//output from rbtree
    rb_test_output(&rb_test_root);
}
