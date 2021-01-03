// SPDX-License-Identifier: GPL-2.0-only
#include <linux/sort.h>
#include <linux/slab.h>
#include <linux/module.h>

#include <linux/export.h>
#include "test/debug.h"
#include "test/define-usr.h"

/* a simple boot-time regression test */

#define TEST_LEN 1000

static int __init cmpint(const void *a, const void *b)
{
	return *(int *)a - *(int *)b;
}

static int __init test_sort_init(void)
{
	int *a, i, r = 1, err = -ENOMEM;

	a = kmalloc_array(TEST_LEN, sizeof(*a), GFP_KERNEL);
	if (!a)
		return err;

    printf("Random Source Data[%d] ---->\n", TEST_LEN);
    for (i = 0; i < TEST_LEN; i++) {
		r = (r * 725861) % 6599;
		a[i] = r;
        printf("%d, ", a[i]);
    }

	sort(a, TEST_LEN, sizeof(*a), cmpint, NULL);

    printf("\n\n");
    printf("Sorting Data ---->\n");

    err = -EINVAL;
    for (i = 0; i < TEST_LEN-1; i++) {
        printf("%d, ", a[i]);

        if (a[i] > a[i+1]) {
            pr_err("Sorting test has failed.\n");
			goto exit;
		}
    }
	err = 0;
    pr_info("Sorting test passed.\n");

exit:
	kfree(a);
	return err;
}

static void __exit test_sort_exit(void)
{
}

void lib_sort_test(void)
{
    int err;
    err = test_sort_init();
    pr_info("err = %d\n", err);
}
EXPORT_SYMBOL(lib_sort_test);

//module_init(test_sort_init);
//module_exit(test_sort_exit);
//MODULE_LICENSE("GPL");
