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

//struct rb_root rb_test_root = RB_ROOT;

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

static void rb_test_insert(struct rb_root *root, struct rb_test_struct *new)
{
	struct rb_node **link = &root->rb_node, *parent = NULL;
	int value = new->value;

	//go to the bottom of the tree
	while (*link) {
	    parent = *link;
        struct rb_test_struct *this = rb_entry(parent, struct rb_test_struct, node);

	    if (this->value > value)
			link = &(*link)->rb_left;
	    else
			link = &(*link)->rb_right;
	}

	//put the new node there
	rb_link_node(&(new->node), parent, link);
	rb_insert_color(&(new->node), root);
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
        free(rb_entry(node, struct rb_test_struct, node));
    rb_test_root->rb_node = NULL;
	printf("\n");
}

void rbtree_test01(void)
{
    struct rb_root rb_test_root = RB_ROOT;
    struct rb_test_struct *new;
	struct rb_node *node;
	int i;

	//insert to rbtree
	for (i = 0; i < 20; i++) {
        new = malloc(sizeof(struct rb_test_struct));
		new->value = i%8;
        rb_test_insert(&rb_test_root, new);
	}
	//output from rbtree
    rb_test_output(&rb_test_root);

    new = rb_test_search(&rb_test_root, 0);
	if (new) {
        printf ("\t value=%d, ", new->value);
        rb_erase(&new->node, &rb_test_root);
		free(new);
		printf("rb_erase.\n");
	}
    else printf ("\t Cannot find!\n");

    rb_test_output(&rb_test_root);
    rb_test_drop(&rb_test_root);
    rb_test_output(&rb_test_root);

	//insert to rbtree
	for (i = 0; i < 20; i++) {
        new = malloc(sizeof(struct rb_test_struct));
		new->value = i%8;
        rb_test_insert(&rb_test_root, new);
	}
	//output from rbtree
    rb_test_output(&rb_test_root);
}

