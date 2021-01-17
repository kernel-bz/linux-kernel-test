// SPDX-License-Identifier: GPL-2.0-only
#include <stdio.h>
#include <stdio_ext.h>

#include <linux/sort.h>
#include <linux/slab.h>
#include <linux/module.h>

#include <linux/export.h>
#include "test/debug.h"
#include "test/define-usr.h"

/* a simple boot-time regression test */

static int __init cmpint(const void *a, const void *b)
{
	return *(int *)a - *(int *)b;
}

static int __init test_sort_init(int cnt)
{
	int *a, i, r = 1, err = -ENOMEM;

    a = kmalloc_array(cnt, sizeof(*a), GFP_KERNEL);
	if (!a)
		return err;

    printf("Random Source Data[%d] ---->\n", cnt);
    for (i = 0; i < cnt; i++) {
		r = (r * 725861) % 6599;
		a[i] = r;
        printf("%d, ", a[i]);
    }

    sort(a, cnt, sizeof(*a), cmpint, NULL);

    printf("\n\n");
    printf("Sorting Data[%d] ---->\n", cnt);

    err = -EINVAL;
    for (i = 0; i < cnt-1; i++) {
        printf("%d, ", a[i]);

        if (a[i] > a[i+1]) {
            pr_err("Sorting test has failed.\n");
			goto exit;
		}
    }
	err = 0;
    printf("\n");
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
    int err, cnt;
    __fpurge(stdin);
    printf("Enter Sorting Data Counter: ");
    scanf("%d", &cnt);
    err = test_sort_init(cnt);
    pr_info("err = %d\n", err);
}
EXPORT_SYMBOL(lib_sort_test);

//module_init(test_sort_init);
//module_exit(test_sort_exit);
//MODULE_LICENSE("GPL");
