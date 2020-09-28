#ifndef __TEST_RADIX_TREE_H
#define __TEST_RADIX_TREE_H

#include <stdio.h>
#include <linux/kernel.h>
#include <linux/radix-tree.h>

extern int kmalloc_verbose;
extern int test_verbose;

static inline void trace_call_rcu(struct rcu_head *head,
		void (*func)(struct rcu_head *head))
{
	if (kmalloc_verbose)
		printf("Delaying free of %p to slab\n", (char *)head -
				offsetof(struct radix_tree_node, rcu_head));
    //call_rcu(head, func);
}

#define printv(verbosity_level, fmt, ...) \
	if(test_verbose >= verbosity_level) \
		printf(fmt, ##__VA_ARGS__)

#undef call_rcu
#define call_rcu(x, y) trace_call_rcu(x, y)

#endif
