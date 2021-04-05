// SPDX-License-Identifier: GPL-2.0+
/*
 * xarray.c: Userspace shim for XArray test-suite
 * Copyright (c) 2018 Matthew Wilcox <willy@infradead.org>
 */

#define XA_DEBUG

#include "test/debug.h"
#include "test/test.h"
#include "xa-test.h"
#include <linux/xarray.h>

#define module_init(x)
#define module_exit(x)
#define MODULE_AUTHOR(x)
#define MODULE_LICENSE(x)
#define dump_stack()	assert(0)

//#include "../../../lib/xarray.c"
#undef XA_DEBUG
//#include "../../../lib/test_xarray.c"

#define XA_BUG_ON(xa, x) do {							\
    if (x) {											\
        pr_out("BUG at %s:%d\n", __func__, __LINE__);	\
    }													\
} while (0)

void xa_debug_xarray_view(struct xarray *xa, u64 index, void *entry)
{
    pr_fn_start_enable(stack_depth);

    pr_view_enable(stack_depth, "%10s : %lu\n", index);
    pr_view_enable(stack_depth, "%10s : 0x%lX\n", entry);
    pr_view_enable(stack_depth, "%10s : %p\n", xa);

    pr_view_enable(stack_depth, "%20s : %p\n", xa->xa_lock);
    pr_view_enable(stack_depth, "%20s : %d\n", xa->xa_flags);
    pr_view_enable(stack_depth, "%20s : %p\n", xa->xa_head);

    pr_fn_end_enable(stack_depth);
}

void xa_debug_state_view(struct xa_state *xas, void *entry)
{
    pr_fn_start_enable(stack_depth);

    pr_view_enable(stack_depth, "%10s : 0x%lX\n", entry);
    pr_view_enable(stack_depth, "%10s : %p\n", xas);

    if(!xas) return;

    pr_view_enable(stack_depth, "%20s : %p\n", xas->xa);
    pr_view_enable(stack_depth, "%20s : %p\n", xas->xa->xa_head);
    pr_view_enable(stack_depth, "%20s : %lu\n", xas->xa_index);
    pr_view_enable(stack_depth, "%20s : %d\n", xas->xa_shift);
    pr_view_enable(stack_depth, "%20s : %d\n", xas->xa_sibs);
    pr_view_enable(stack_depth, "%20s : %d\n", xas->xa_offset);
    pr_view_enable(stack_depth, "%20s : %d\n", xas->xa_pad);
    pr_view_enable(stack_depth, "%20s : %p\n", xas->xa_node);
    pr_view_enable(stack_depth, "%20s : %p\n", xas->xa_alloc);
    pr_view_enable(stack_depth, "%20s : %p\n", xas->xa_update);

    pr_fn_end_enable(stack_depth);
}

void xa_debug_node_view(struct xa_node *node, void *entry)
{
    pr_fn_start_enable(stack_depth);

    pr_view_enable(stack_depth, "%10s : 0x%lX\n", entry);
    pr_view_enable(stack_depth, "%10s : %p\n", node);

    //if (!xa_is_node(node) || xa_is_err(node)) return;
    if ((unsigned long)node <= MAX_ERRNO || IS_ERR(node)) return;

    pr_view_enable(stack_depth, "%20s : %p\n", node->array);
    pr_view_enable(stack_depth, "%20s : %p\n", node->parent);
    pr_view_enable(stack_depth, "%20s : %p\n", node->slots);
    pr_view_enable(stack_depth, "%20s : %d\n", node->shift);
    pr_view_enable(stack_depth, "%20s : %d\n", node->offset);
    pr_view_enable(stack_depth, "%20s : %d\n", node->count);
    pr_view_enable(stack_depth, "%20s : %d\n", node->nr_values);
    pr_out_enable(stack_depth, "\t[ ");
    int i, j;
    for(i=0; i<XA_CHUNK_SIZE; i++)
        printf("0x%lX, ", node->slots[i]);
    printf("]\n");
    pr_out_enable(stack_depth, "\t[ ");
    for(i=0; i<XA_MAX_MARKS; i++) {
        for(j=0; j<XA_MARK_LONGS; j++) {
            printf("0x%lX:", node->tags[i][j]);
            printf("0x%lX, ", node->marks[i][j]);
        }
    }
    printf("]\n");
    pr_fn_end_enable(stack_depth);
}

void xa_debug_node_print(struct xarray *xa)
{
    void *entry, *onode = NULL;
    XA_STATE(xas, xa, 0);

    xa_lock(xa);
    xas_for_each(&xas, entry, ULONG_MAX) {
        if (onode != xas.xa_node) {
            printf("\t]\n\t");
            printf("parent=%p, prev=%p, node=%p, next=%p\n"
                   , xas.xa_node->parent
                   , xas.xa_node->prev
                   , xas.xa_node
                   , xas.xa_node->next
                   );
            printf("\t[ ");
        }
        printf("%lu:%p, ", xas.xa_index, entry);
        onode = xas.xa_node;
    }
    printf("]\n\n");
    xa_unlock(xa);
}

void xa_constants_view(void)
{
    pr_fn_start(stack_depth);

    void *p, *entry;
    unsigned long index, value, internal;

    pr_view(stack_depth, "%30s : %d\n", BITS_PER_LONG);
    pr_view(stack_depth, "%30s : %d\n", BITS_PER_XA_VALUE);
    pr_view(stack_depth, "%30s : 0x%llX\n", ULONG_MAX);
    pr_view(stack_depth, "%30s : 0x%llX\n", LONG_MAX);
    printf("\n");

    p = xa_mk_internal(-MAX_ERRNO);
    pr_view(stack_depth, "%30s : %d\n", MAX_ERRNO);	//4095(0xFFF)
    pr_view(stack_depth, "%30s : 0x%llX\n", MAX_ERRNO);
    pr_view(stack_depth, "%30s : 0x%llX\n", -MAX_ERRNO);
    pr_view(stack_depth, "%30s : 0x%llX\n", (unsigned long)-MAX_ERRNO);
    pr_view(stack_depth, "%30s : %p\n", xa_mk_internal(-MAX_ERRNO));
    pr_view(stack_depth, "%30s : 0x%llX\n", (unsigned long)p);
    printf("\n");

    pr_view(stack_depth, "%30s : 0x%X\n", XA_ZERO_ENTRY);	//xa_mk_internal(257)
    pr_view(stack_depth, "%30s : 0x%X\n", XA_FLAGS_MARK(XA_MARK_0));
    pr_view(stack_depth, "%30s : 0x%X\n", XA_FLAGS_MARK(XA_MARK_1));
    pr_view(stack_depth, "%30s : 0x%X\n", XA_FLAGS_MARK(XA_MARK_2));
    pr_view(stack_depth, "%30s : 0x%X\n", XA_FLAGS_ALLOC);
    pr_view(stack_depth, "%30s : 0x%X\n", XA_FLAGS_ALLOC1);
    printf("\n");

    pr_view(stack_depth, "%30s : %d\n", XA_CHUNK_SHIFT);
    pr_view(stack_depth, "%30s : %d\n", XA_CHUNK_SIZE);
    pr_view(stack_depth, "%30s : 0x%X\n", XA_CHUNK_MASK);
    pr_view(stack_depth, "%30s : %d\n", XA_MAX_MARKS);
    pr_view(stack_depth, "%30s : %d\n", XA_MARK_LONGS);
    printf("\n");

    for (index=0; index<10; index++) {
        p = xa_mk_value(index & LONG_MAX);
        value = xa_to_value(p);
        entry = xa_mk_internal(index);
        internal = xa_to_internal(entry);
        printf("\t\t%u: value=0x%llx[%p], internal=0x%llx[%p]\n"
               , index, value, p, internal, entry);
    }

    pr_fn_end(stack_depth);
}

static DEFINE_XARRAY(xa1);

static void _xa_store_erase_find_test(struct xarray *xa, int cnt)
{
    pr_fn_start_enable(stack_depth);

    //XA_BUG_ON(xa, xa_err(xa_store(xa, 0, xa_mk_value(0), GFP_NOWAIT)) != 0);
    //XA_BUG_ON(xa, xa_err(xa_store(xa, 1, xa_mk_value(1), GFP_NOWAIT)) != 0);

    unsigned long index = 0;
    for (index=0; index<cnt; index++) {
        //xa_store(xa, index, xa_mk_value(index), GFP_NOWAIT);	//error
        if (xa_err(xa_store(xa, index, xa_mk_value(index), GFP_KERNEL)))
            return;
    }

#if 0
    index = 7;
    XA_STATE(xas, xa, index);
    //void *entry;

    //XA_BUG_ON(xa, xa_err(xa_erase(xa, index)) != index);	//bug

    XA_BUG_ON(xa, xas_find(&xas, ULONG_MAX) != xa_mk_value(index));

    entry = xa_find(xa, &index, ULONG_MAX, XA_PRESENT);
    pr_view_enable(stack_depth, "%s : %p\n", entry);
    pr_view_enable(stack_depth, "%s : 0x%lX\n", xa_to_value(entry));
#endif

    xa_debug_node_print(xa);

    pr_fn_end_enable(stack_depth);
}

void xarray_simple_test(void)
{
    int cnt;

    __fpurge(stdin);
    printf("Enter XArray Testing Counter: ");
    scanf("%d", &cnt);

    radix_tree_init();

    _xa_store_erase_find_test(&xa1, cnt);
}

void xarray_tests(void)
{
    //lib/test-xarray.c
    lib_xarray_test();	//xarray_checks()
}

//int __weak main(void)
void xarray_test_run(void)
{
	radix_tree_init();
	xarray_tests();
	radix_tree_cpu_dead(1);
	rcu_barrier();
	if (nr_allocated)
		printf("nr_allocated = %d\n", nr_allocated);
	return 0;
}
