// SPDX-License-Identifier: GPL-2.0+
/*
 * lib/test_ida.c: Test the IDA API
 * Copyright (c) 2016-2018 Microsoft Corporation
 * Copyright (c) 2018 Oracle Corporation
 * Author: Matthew Wilcox <willy@infradead.org>
 */

#include "test/user-define.h"
#include "test/debug.h"

#include <linux/idr.h>
#include <linux/module.h>
#include <linux/export.h>

static unsigned int tests_run;
static unsigned int tests_passed;

#ifdef __KERNEL__
void ida_dump(struct ida *ida) { }
#endif
#define IDA_BUG_ON(ida, x) do {		\
    tests_run++;					\
    if (x) {						\
        ida_dump(ida);				\
        dump_stack();				\
    } else {						\
        tests_passed++;				\
	}								\
} while (0)

/*
 * Straightforward checks that allocating and freeing IDs work.
 */
static void ida_check_alloc(struct ida *ida)
{
    //pr_fn_start_on(stack_depth);
	int i, id;

    for (i = 0; i < 10000; i++) {
        IDA_BUG_ON(ida, ida_alloc(ida, GFP_KERNEL) != i);
        //printf("%d, ", i);
    }
    //[0..9999]

	ida_free(ida, 20);
	ida_free(ida, 21);
	for (i = 0; i < 3; i++) {
		id = ida_alloc(ida, GFP_KERNEL);
		IDA_BUG_ON(ida, id < 0);
        if (i == 2)
            IDA_BUG_ON(ida, id != 10000);  //[0..20.21....10000]
    }

    for (i = 0; i < 5000; i++)
        ida_free(ida, i);		//[0..4999] [5000...10000]

    IDA_BUG_ON(ida, ida_alloc_min(ida, 5000, GFP_KERNEL) != 10001);
    //[5000...10000..10001]
    ida_destroy(ida);

	IDA_BUG_ON(ida, !ida_is_empty(ida));

    //pr_fn_end_on(stack_depth);
}

/* Destroy an IDA with a single entry at @base */
static void ida_check_destroy_1(struct ida *ida, unsigned int base)
{
	IDA_BUG_ON(ida, ida_alloc_min(ida, base, GFP_KERNEL) != base);
	IDA_BUG_ON(ida, ida_is_empty(ida));
	ida_destroy(ida);
	IDA_BUG_ON(ida, !ida_is_empty(ida));
}

/* Check that ida_destroy and ida_is_empty work */
static void ida_check_destroy(struct ida *ida)
{
	/* Destroy an already-empty IDA */
	IDA_BUG_ON(ida, !ida_is_empty(ida));
	ida_destroy(ida);
	IDA_BUG_ON(ida, !ida_is_empty(ida));

	ida_check_destroy_1(ida, 0);
	ida_check_destroy_1(ida, 1);
    ida_check_destroy_1(ida, 1023);
    ida_check_destroy_1(ida, 1024);
    ida_check_destroy_1(ida, 12345678);
}

/*
 * Check what happens when we fill a leaf and then delete it.  This may
 * discover mishandling of IDR_FREE.
 */
static void ida_check_leaf(struct ida *ida, unsigned int base)
{
	unsigned long i;

	for (i = 0; i < IDA_BITMAP_BITS; i++) {
		IDA_BUG_ON(ida, ida_alloc_min(ida, base, GFP_KERNEL) !=
				base + i);
	}

	ida_destroy(ida);
	IDA_BUG_ON(ida, !ida_is_empty(ida));

	IDA_BUG_ON(ida, ida_alloc(ida, GFP_KERNEL) != 0);
	IDA_BUG_ON(ida, ida_is_empty(ida));
	ida_free(ida, 0);
	IDA_BUG_ON(ida, !ida_is_empty(ida));
}

/*
 * Check allocations up to and slightly above the maximum allowed (2^31-1) ID.
 * Allocating up to 2^31-1 should succeed, and then allocating the next one
 * should fail.
 */
static void ida_check_max(struct ida *ida)
{
	unsigned long i, j;

	for (j = 1; j < 65537; j *= 2) {
        unsigned long base = (1UL << 31) - j;
        //2^31(2G), ....... 0
		for (i = 0; i < j; i++) {
			IDA_BUG_ON(ida, ida_alloc_min(ida, base, GFP_KERNEL) !=
					base + i);
		}
		IDA_BUG_ON(ida, ida_alloc_min(ida, base, GFP_KERNEL) !=
				-ENOSPC);
		ida_destroy(ida);
		IDA_BUG_ON(ida, !ida_is_empty(ida));
	}
}

/*
 * Check handling of conversions between exceptional entries and full bitmaps.
 */
static void ida_check_conv(struct ida *ida)
{
	unsigned long i;

	for (i = 0; i < IDA_BITMAP_BITS * 2; i += IDA_BITMAP_BITS) {
        //i+1....-->
        IDA_BUG_ON(ida, ida_alloc_min(ida, i + 1, GFP_KERNEL) != i + 1);
        //i+BITS_PER_LONG....==>
		IDA_BUG_ON(ida, ida_alloc_min(ida, i + BITS_PER_LONG,
					GFP_KERNEL) != i + BITS_PER_LONG);
		ida_free(ida, i + 1);
		ida_free(ida, i + BITS_PER_LONG);
		IDA_BUG_ON(ida, !ida_is_empty(ida));
	}

	for (i = 0; i < IDA_BITMAP_BITS * 2; i++)
		IDA_BUG_ON(ida, ida_alloc(ida, GFP_KERNEL) != i);
	for (i = IDA_BITMAP_BITS * 2; i > 0; i--)
		ida_free(ida, i - 1);
	IDA_BUG_ON(ida, !ida_is_empty(ida));

	for (i = 0; i < IDA_BITMAP_BITS + BITS_PER_LONG - 4; i++)
		IDA_BUG_ON(ida, ida_alloc(ida, GFP_KERNEL) != i);
	for (i = IDA_BITMAP_BITS + BITS_PER_LONG - 4; i > 0; i--)
		ida_free(ida, i - 1);
	IDA_BUG_ON(ida, !ida_is_empty(ida));
}

static DEFINE_IDA(ida);

static int ida_checks(void)
{
	IDA_BUG_ON(&ida, !ida_is_empty(&ida));

    printk("ida_check_alloc()...\n");
	ida_check_alloc(&ida);
    printk("ida_check_destroy()...\n");
    ida_check_destroy(&ida);
    printk("ida_check_leaf(0)...\n");
    ida_check_leaf(&ida, 0);
    printk("ida_check_leaf(1024)...\n");
    ida_check_leaf(&ida, 1024);
    printk("ida_check_leaf(1024 * 64)...\n");
    ida_check_leaf(&ida, 1024 * 64);
    printk("ida_check_max()...\n");
    ida_check_max(&ida);
    printk("ida_check_conv()...\n");
    ida_check_conv(&ida);

	printk("IDA: %u of %u tests passed\n", tests_passed, tests_run);
    //return (tests_run != tests_passed) ? 0 : -EINVAL;
    return (tests_run == tests_passed) ? 0 : -EINVAL;
}

static void ida_exit(void)
{
}

void lib_ida_test(void)
{
    int ret;
    radix_tree_init();
    ret = ida_checks();
    pr_info("result = %d\n", ret);
}
EXPORT_SYMBOL(lib_ida_test);

//module_init(ida_checks);
//module_exit(ida_exit);
//MODULE_AUTHOR("Matthew Wilcox <willy@infradead.org>");
//MODULE_LICENSE("GPL");
