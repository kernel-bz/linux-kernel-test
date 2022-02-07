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
        _pr_warn("BUG at xa=%p:%s:%d\n", xa, __func__, __LINE__);	\
    }													\
} while (0)

void xa_debug_xarray_view(struct xarray *xa, u64 index, void *entry)
{
    pr_fn_start_enable(stack_depth);

    pr_view_enable(stack_depth, "%10s : %lu\n", index);
    pr_view_enable(stack_depth, "%10s : 0x%lX\n", entry);
    pr_view_enable(stack_depth, "%10s : %p\n", xa);

    pr_view_enable(stack_depth, "%20s : %p\n", xa->xa_lock);
    pr_view_enable(stack_depth, "%20s : %p\n", xa->xa_flags);
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
    pr_view_enable(stack_depth, "%20s : %p\n", &xas->xa->xa_head);
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
    pr_view_enable(stack_depth, "%20s : %d\n\t", node->nr_values);
    pr_out_enable(stack_depth, "slots[ ");
    int i, j;
    for(i=0; i<XA_CHUNK_SIZE; i++)
        printf("0x%lX, ", node->slots[i]);
    printf("]\n\t");
    pr_out_enable(stack_depth, "marks[ ");
    for(i=0; i<XA_MAX_MARKS; i++) {
        for(j=0; j<XA_MARK_LONGS; j++) {
            //printf("0x%lX:", node->tags[i][j]);
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

    pr_fn_start_enable(stack_depth);

    xa_lock(xa);
    pr_out_on(stack_depth, ">>> %s\n", "xa_node total outputs...");
    xas_for_each(&xas, entry, ULONG_MAX) {
        if (onode != xas.xa_node) {
            printf("\n\t[ parent=%p, prev=%p, node=%p, next=%p ]\n"
                   , xas.xa_node->parent
                   , xas.xa_node->prev
                   , xas.xa_node
                   , xas.xa_node->next
                   );
            printf("\t[ tags.u.marks: 0x%lX, 0x%lX, 0x%lX ]\t\n\t",
                    xas.xa_node->marks[XA_MARK_0][0],
                    xas.xa_node->marks[XA_MARK_1][0],
                    xas.xa_node->marks[XA_MARK_2][0]
                   );
        }
        printf("[ %d:%lu:%p ] ", xas.xa_offset, xas.xa_index, entry);
        onode = xas.xa_node;
    }
    xa_unlock(xa);

    pr_view_enable(stack_depth, "%40s : %u\n",
                        xa_to_node(xa_head(xa))->count);
    pr_view_enable(stack_depth, "%40s : %u\n",
                        xa_to_node(xa_head(xa))->nr_values);

    pr_fn_end_enable(stack_depth);
}

void xa_debug_node_print_range(struct xarray *xa)
{
    void *entry, *onode = NULL;
    XA_STATE_ORDER(xas, xa, 0, 0);

    pr_fn_start_enable(stack_depth);

    xa_lock(xa);
    pr_out_on(stack_depth, ">>> %s\n", "xa_node total outputs over range...");
    //xas_for_each_conflict(&xas, entry) {
    xas_for_range(&xas, entry, ULONG_MAX) {
        if (onode != xas.xa_node) {
            printf("\n\t[ parent=%p, prev=%p, node=%p, next=%p ]\n"
                   , xas.xa_node->parent
                   , xas.xa_node->prev
                   , xas.xa_node
                   , xas.xa_node->next
                   );
            printf("\t[ tags.u.marks: 0x%lX, 0x%lX, 0x%lX ]\t\n\t",
                    xas.xa_node->marks[XA_MARK_0][0],
                    xas.xa_node->marks[XA_MARK_1][0],
                    xas.xa_node->marks[XA_MARK_2][0]
                   );
        }
        printf("[ %d:%lu:%p ] ", xas.xa_offset, xas.xa_index, entry);
        onode = xas.xa_node;
    }
    xa_unlock(xa);

    pr_view_enable(stack_depth, "%40s : %u\n",
                        xa_to_node(xa_head(xa))->count);
    pr_view_enable(stack_depth, "%40s : %u\n",
                        xa_to_node(xa_head(xa))->nr_values);

    pr_fn_end_enable(stack_depth);
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

    pr_view(stack_depth, "%30s : 0x%X\n", XA_RETRY_ENTRY);	//xa_mk_internal(256)
    pr_view(stack_depth, "%30s : 0x%X\n", XA_ZERO_ENTRY);	//xa_mk_internal(257)
    pr_view(stack_depth, "%30s : 0x%X\n", XA_FLAGS_MARK(XA_MARK_0));
    pr_view(stack_depth, "%30s : 0x%X\n", XA_FLAGS_MARK(XA_MARK_1));
    pr_view(stack_depth, "%30s : 0x%X\n", XA_FLAGS_MARK(XA_MARK_2));
    pr_view(stack_depth, "%30s : 0x%X\n", XA_FLAGS_ALLOC);
    pr_view(stack_depth, "%30s : 0x%X\n", XA_FLAGS_ALLOC1);
    printf("\n");

    pr_view(stack_depth, "%30s : %d\n", sizeof(struct xa_node));

    pr_view(stack_depth, "%30s : %d\n", XA_CHUNK_SHIFT);
    pr_view(stack_depth, "%30s : %d\n", XA_CHUNK_SIZE);
    pr_view(stack_depth, "%30s : 0x%X\n", XA_CHUNK_MASK);
    pr_view(stack_depth, "%30s : 0x%X\n", ~XA_CHUNK_MASK);
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

static void _xa_store_find_erase_test(struct xarray *xa, int first, int last, int step)
{
    pr_fn_start_enable(stack_depth);

    //XA_BUG_ON(xa, xa_err(xa_store(xa, 0, xa_mk_value(0), GFP_NOWAIT)) != 0);
    unsigned long index;
    for (index = first; index <= last; index += step) {
        pr_out_on(stack_depth, ">>> %u: %s\n", index, "xa_store() testing...");
        if (xa_err(xa_store(xa, index, xa_mk_value(index), GFP_KERNEL)))
        //if (xa_err(xa_store(xa, index, xa_mk_value(index), GFP_NOWAIT)))
            return;
    }

    xa_debug_node_print(xa);

    void *entry;
    index = first;
    //for (index = first; index <= last; index += step) {
        pr_out_on(stack_depth, ">>> %u: %s\n", index, "xa_find() testing...");
        pr_view_enable(stack_depth, "%10s : %u\n", index);
        entry = xa_find(xa, &index, ULONG_MAX, XA_PRESENT);
        pr_view_enable(stack_depth, "%20s : %p\n", entry);
        pr_view_enable(stack_depth, "%20s : %d\n\n", xa_to_value(entry));
    //}

    //for (index = 16; index <= last; index += step) {
        pr_out_on(stack_depth, ">>> %u: %s\n", index, "xa_erase() testing...");
        entry = xa_erase(xa, index);
        //if (xa_err(entry)) break;
        XA_BUG_ON(xa, xa_to_value(entry) != index);
    //}

    //xa_debug_node_print(xa);

    pr_fn_end_enable(stack_depth);
}

void xarray_test_simple(void)
{
    int first, last, step;

    pr_fn_start_enable(stack_depth);

    __fpurge(stdin);
    printf("Input first index for testing: ");
    scanf("%d", &first);
    printf("Input last index for testing: ");
    scanf("%d", &last);
    printf("Input step size for testing: ");
    scanf("%d", &step);

    pr_view_enable(stack_depth, "%s : %p\n", &xa1);

    radix_tree_init();

    _xa_store_find_erase_test(&xa1, first, last, step);

    pr_fn_end_enable(stack_depth);
}

void xarray_test_store_range(void)
{
    unsigned long first, last, index;

    pr_fn_start_enable(stack_depth);

    __fpurge(stdin);
    printf("Enter range first index: ");
    scanf("%lu", &first);
    printf("Enter range last index: ");
    scanf("%lu", &last);
    printf("Enter index to find: ");
    scanf("%lu", &index);

    pr_view_enable(stack_depth, "%10s : %p\n", &xa1);

    radix_tree_init();

    xa_store_range(&xa1, first, last, xa_mk_value(first & LONG_MAX), GFP_KERNEL);

    xa_debug_node_print_range(&xa1);

    void *entry;
    entry = xa_find(&xa1, &index, ULONG_MAX, XA_PRESENT);
    pr_view_enable(stack_depth, "%10s : %p\n", entry);

    XA_STATE(xas, &xa1, index);
    entry = xas_find_conflict(&xas);
    pr_view_enable(stack_depth, "%10s : %p\n", entry);

    pr_fn_end_enable(stack_depth);
}

void xarray_test_marks(void)
{
    unsigned long index, count;
    void *entry;

    pr_fn_start_enable(stack_depth);

    __fpurge(stdin);
    printf("Enter index count: ");
    scanf("%lu", &count);

    radix_tree_init();

    for (index = 0; index < count; index++)
        entry = xa_store(&xa1, index, xa_mk_value(index), GFP_KERNEL);

    for (index = 0; index < count; index += 2) {
        xa_set_mark(&xa1, index, XA_MARK_0);
        xa_set_mark(&xa1, index, XA_MARK_1);
        xa_set_mark(&xa1, index, XA_MARK_2);
    }
    xa_debug_node_print(&xa1);

    for (index = 0; index < count; index += 2)
        if (xa_get_mark(&xa1, index, XA_MARK_0))
            xa_clear_mark(&xa1, index, XA_MARK_0);

    for (index = 0; index < count; index += 4)
        if (xa_get_mark(&xa1, index, XA_MARK_1))
            xa_clear_mark(&xa1, index, XA_MARK_1);

    for (index = 0; index < count; index += 8)
        if (xa_get_mark(&xa1, index, XA_MARK_2))
            xa_clear_mark(&xa1, index, XA_MARK_2);

    xa_debug_node_print(&xa1);
    //xa_debug_node_print_range(&xa1);

    XA_STATE(xas, &xa1, 0);

    rcu_read_lock();
    xas_for_each_marked(&xas, entry, ULONG_MAX, XA_MARK_2) {
        if (xas_retry(&xas, entry))
            continue;
        pr_view_enable(stack_depth, "%20s : %d\n", xas.xa_index);
        pr_view_enable(stack_depth, "%20s : %p\n", entry);
    }
    rcu_read_unlock();

    pr_fn_end_enable(stack_depth);
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
