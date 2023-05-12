// SPDX-License-Identifier: GPL-2.0-only
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/rbtree_augmented.h>

//#include <linux/random.h>
#include <linux/random_.h>

#include <linux/slab.h>

//#include <asm/timex.h>
#include <asm-generic/timex.h>
#include <include/linux/acpi.h>

#include "test/algorithm/rbtree/rbtest.h"
#include "test/user-common.h"

#define __param(type, name, init, msg)		\
	static type name = init;		\
	module_param(name, type, 0444);		\
	MODULE_PARM_DESC(name, msg);

__param(int, nnodes, 100, "Number of Nodes in the rb-tree");
__param(int, perf_loops, 1000, "Number of iterations modifying the rb-tree");
__param(int, check_loops, 100, "Number of iterations modifying and verifying the rb-tree");

struct test_node {
	u32 key;
	struct rb_node rb;

	/* following fields used for testing augmented rbtree functionality */
	u32 val;
	u32 augmented;
};

static struct rb_root_cached root = RB_ROOT_CACHED;
static struct test_node *Nodes = NULL;

static struct rnd_state rnd;

static void insert(struct test_node *node, struct rb_root_cached *root)
{
	struct rb_node **new = &root->rb_root.rb_node, *parent = NULL;
	u32 key = node->key;

	while (*new) {
		parent = *new;
		if (key < rb_entry(parent, struct test_node, rb)->key)
			new = &parent->rb_left;
		else
			new = &parent->rb_right;
	}

	rb_link_node(&node->rb, parent, new);
	rb_insert_color(&node->rb, &root->rb_root);
}

static void insert_cached(struct test_node *node, struct rb_root_cached *root)
{
	struct rb_node **new = &root->rb_root.rb_node, *parent = NULL;
	u32 key = node->key;
	bool leftmost = true;

	while (*new) {
		parent = *new;
		if (key < rb_entry(parent, struct test_node, rb)->key)
			new = &parent->rb_left;
		else {
			new = &parent->rb_right;
			leftmost = false;
		}
	}

	rb_link_node(&node->rb, parent, new);
	rb_insert_color_cached(&node->rb, root, leftmost);
}

static inline void erase(struct test_node *node, struct rb_root_cached *root)
{
	rb_erase(&node->rb, &root->rb_root);
}

static inline void erase_cached(struct test_node *node, struct rb_root_cached *root)
{
	rb_erase_cached(&node->rb, root);
}


#define NODE_VAL(node) ((node)->val)

RB_DECLARE_CALLBACKS_MAX(static, augment_callbacks,
			 struct test_node, rb, u32, augmented, NODE_VAL)

static void insert_augmented(struct test_node *node,
			     struct rb_root_cached *root)
{
	struct rb_node **new = &root->rb_root.rb_node, *rb_parent = NULL;
	u32 key = node->key;
	u32 val = node->val;
	struct test_node *parent;

	while (*new) {
		rb_parent = *new;
		parent = rb_entry(rb_parent, struct test_node, rb);
		if (parent->augmented < val)
			parent->augmented = val;
		if (key < parent->key)
			new = &parent->rb.rb_left;
		else
			new = &parent->rb.rb_right;
	}

	node->augmented = val;
	rb_link_node(&node->rb, rb_parent, new);
	rb_insert_augmented(&node->rb, &root->rb_root, &augment_callbacks);
}

static void insert_augmented_cached(struct test_node *node,
				    struct rb_root_cached *root)
{
	struct rb_node **new = &root->rb_root.rb_node, *rb_parent = NULL;
	u32 key = node->key;
	u32 val = node->val;
	struct test_node *parent;
	bool leftmost = true;

	while (*new) {
		rb_parent = *new;
		parent = rb_entry(rb_parent, struct test_node, rb);
		if (parent->augmented < val)
			parent->augmented = val;
		if (key < parent->key)
			new = &parent->rb.rb_left;
		else {
			new = &parent->rb.rb_right;
			leftmost = false;
		}
	}

	node->augmented = val;
	rb_link_node(&node->rb, rb_parent, new);
	rb_insert_augmented_cached(&node->rb, root,
				   leftmost, &augment_callbacks);
}


static void erase_augmented(struct test_node *node, struct rb_root_cached *root)
{
	rb_erase_augmented(&node->rb, &root->rb_root, &augment_callbacks);
}

static void erase_augmented_cached(struct test_node *node,
				   struct rb_root_cached *root)
{
	rb_erase_augmented_cached(&node->rb, root, &augment_callbacks);
}

static void init(void)
{
	int i;
	for (i = 0; i < nnodes; i++) {
        Nodes[i].key = prandom_u32_state(&rnd);
        Nodes[i].val = prandom_u32_state(&rnd);
	}
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

static void check_postorder_foreach(int nr_nodes)
{
	struct test_node *cur, *n;
	int count = 0;
	rbtree_postorder_for_each_entry_safe(cur, n, &root.rb_root, rb)
		count++;

	WARN_ON_ONCE(count != nr_nodes);
}

static void check_postorder(int nr_nodes)
{
	struct rb_node *rb;
	int count = 0;
	for (rb = rb_first_postorder(&root.rb_root); rb; rb = rb_next_postorder(rb))
		count++;

	WARN_ON_ONCE(count != nr_nodes);
}

static void check(int nr_nodes)
{
	struct rb_node *rb;
	int count = 0, blacks = 0;
	u32 prev_key = 0;

	for (rb = rb_first(&root.rb_root); rb; rb = rb_next(rb)) {
		struct test_node *node = rb_entry(rb, struct test_node, rb);
		WARN_ON_ONCE(node->key < prev_key);
		WARN_ON_ONCE(is_red(rb) &&
			     (!rb_parent(rb) || is_red(rb_parent(rb))));
		if (!count)
			blacks = black_path_count(rb);
		else
			WARN_ON_ONCE((!rb->rb_left || !rb->rb_right) &&
				     blacks != black_path_count(rb));
		prev_key = node->key;
		count++;
	}

	WARN_ON_ONCE(count != nr_nodes);
	WARN_ON_ONCE(count < (1 << black_path_count(rb_last(&root.rb_root))) - 1);

	check_postorder(nr_nodes);
	check_postorder_foreach(nr_nodes);
}

static void check_augmented(int nr_nodes)
{
	struct rb_node *rb;

	check(nr_nodes);
	for (rb = rb_first(&root.rb_root); rb; rb = rb_next(rb)) {
		struct test_node *node = rb_entry(rb, struct test_node, rb);
		u32 subtree, max = node->val;
		if (node->rb.rb_left) {
			subtree = rb_entry(node->rb.rb_left, struct test_node,
					   rb)->augmented;
			if (max < subtree)
				max = subtree;
		}
		if (node->rb.rb_right) {
			subtree = rb_entry(node->rb.rb_right, struct test_node,
					   rb)->augmented;
			if (max < subtree)
				max = subtree;
		}
		WARN_ON_ONCE(node->augmented != max);
	}
}

//static int __init rbtree_test_init(void)
int rbtree_test_all(void)
{
	int i, j;
    cycles_t time1, time2, time;
    struct rb_node *node;

    pr_fn_start(stack_depth);

    Nodes = kmalloc_array(nnodes, sizeof(*Nodes), GFP_KERNEL);
    if (!Nodes)
		return -ENOMEM;

    pr_out(stack_depth, "Start of rbtree testing from /lib/rbtree_test.c\n");

	prandom_seed_state(&rnd, 3141592653589793238ULL);
	init();

    //time1 = get_cycles();
    time1 = get_time_nsec();

	for (i = 0; i < perf_loops; i++) {
		for (j = 0; j < nnodes; j++)
            insert(Nodes + j, &root);
		for (j = 0; j < nnodes; j++)
            erase(Nodes + j, &root);
	}

    //time2 = get_cycles();
    time2 = get_time_nsec();
    time = time2 - time1;

    pr_view(stack_depth, "Test1 Total(insert-erase): %10s : %llu(ns)\n", time);
    time = div_u64(time, perf_loops);
    pr_view(stack_depth, "Test1 Step(insert-erase) : %10s : %llu(ns)\n\n", time);

    //time1 = get_cycles();
    time1 = get_time_nsec();

	for (i = 0; i < perf_loops; i++) {
		for (j = 0; j < nnodes; j++)
            insert_cached(Nodes + j, &root);
		for (j = 0; j < nnodes; j++)
            erase_cached(Nodes + j, &root);
	}

    //time2 = get_cycles();
    time2 = get_time_nsec();
    time = time2 - time1;

    pr_view(stack_depth, "Test2 Total(cached)      : %10s : %llu(ns)\n", time);
    time = div_u64(time, perf_loops);
    pr_view(stack_depth, "Test2 Step(cached)       : %10s : %llu(ns)\n\n", time);

	for (i = 0; i < nnodes; i++)
        insert(Nodes + i, &root);

    //time1 = get_cycles();
    time1 = get_time_nsec();

	for (i = 0; i < perf_loops; i++) {
		for (node = rb_first(&root.rb_root); node; node = rb_next(node))
			;
	}

    //time2 = get_cycles();
    time2 = get_time_nsec();
    time = time2 - time1;

    pr_view(stack_depth, "Test3 Total(loop)        : %10s : %llu(ns)\n", time);
    time = div_u64(time, perf_loops);
    pr_view(stack_depth, "Test3 Step(loop)         : %10s : %llu(ns)\n\n", time);

    //time1 = get_cycles();
    time1 = get_time_nsec();

	for (i = 0; i < perf_loops; i++)
		node = rb_first(&root.rb_root);

    //time2 = get_cycles();
    time2 = get_time_nsec();
    time = time2 - time1;

    pr_view(stack_depth, "Test4 Total(first)       : %10s : %llu(ns)\n", time);
    time = div_u64(time, perf_loops);
    pr_view(stack_depth, "Test4 Step(first)        : %10s : %llu(ns)\n\n", time);

    //time1 = get_cycles();
    time1 = get_time_nsec();

	for (i = 0; i < perf_loops; i++)
		node = rb_first_cached(&root);

    //time2 = get_cycles();
    time2 = get_time_nsec();
    time = time2 - time1;

    pr_view(stack_depth, "Test5 Total(first cached): %10s : %llu(ns)\n", time);
    time = div_u64(time, perf_loops);
    pr_view(stack_depth, "Test5 Step(first cached) : %10s : %llu(ns)\n\n", time);

	for (i = 0; i < nnodes; i++)
        erase(Nodes + i, &root);

	/* run checks */
	for (i = 0; i < check_loops; i++) {
		init();
		for (j = 0; j < nnodes; j++) {
			check(j);
            insert(Nodes + j, &root);
		}
		for (j = 0; j < nnodes; j++) {
			check(nnodes - j);
            erase(Nodes + j, &root);
		}
		check(0);
	}

    pr_out(stack_depth, "Start of augmented rbtree testing from /lib/rbtree_test.c\n");

	init();

    //time1 = get_cycles();
    time1 = get_time_nsec();

	for (i = 0; i < perf_loops; i++) {
		for (j = 0; j < nnodes; j++)
            insert_augmented(Nodes + j, &root);
		for (j = 0; j < nnodes; j++)
            erase_augmented(Nodes + j, &root);
	}

    //time2 = get_cycles();
    time2 = get_time_nsec();
    time = time2 - time1;

    pr_view(stack_depth, "Test6 Total(insert-erase): %10s : %llu(ns)\n", time);
    time = div_u64(time, perf_loops);
    pr_view(stack_depth, "Test6 Step(insert-erase) : %10s : %llu(ns)\n\n", time);

    //time1 = get_cycles();
    time1 = get_time_nsec();

	for (i = 0; i < perf_loops; i++) {
		for (j = 0; j < nnodes; j++)
            insert_augmented_cached(Nodes + j, &root);
		for (j = 0; j < nnodes; j++)
            erase_augmented_cached(Nodes + j, &root);
	}

    //time2 = get_cycles();
    time2 = get_time_nsec();
    time = time2 - time1;

    pr_view(stack_depth, "Test7 Total(cached)      : %10s : %llu(ns)\n", time);
    time = div_u64(time, perf_loops);
    pr_view(stack_depth, "Test7 Step(cached)       : %10s : %llu(ns)\n\n", time);

	for (i = 0; i < check_loops; i++) {
		init();
		for (j = 0; j < nnodes; j++) {
			check_augmented(j);
            insert_augmented(Nodes + j, &root);
		}
		for (j = 0; j < nnodes; j++) {
			check_augmented(nnodes - j);
            erase_augmented(Nodes + j, &root);
		}
		check_augmented(0);
	}

    kfree(Nodes);

    pr_fn_end(stack_depth);

	return -EAGAIN; /* Fail will directly unload the module */
}

static void __exit rbtree_test_exit(void)
{
	printk(KERN_ALERT "test exit\n");
}

//module_init(rbtree_test_init)
//module_exit(rbtree_test_exit)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michel Lespinasse");
MODULE_DESCRIPTION("Red Black Tree test");
