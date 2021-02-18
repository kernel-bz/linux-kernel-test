// SPDX-License-Identifier: GPL-2.0-only
#define pr_fmt(fmt) "list_sort_test: " fmt

#include <linux/kernel.h>
#include <linux/list_sort.h>
#include <linux/list.h>
#include <linux/module.h>
//#include <linux/printk.h>
#include <linux/slab.h>
//#include <linux/random.h>
#include <linux/random_.h>

#include <linux/export.h>
#include "test/debug.h"

/*
 * The pattern of set bits in the list length determines which cases
 * are hit in list_sort().
 */
#define TEST_LIST_LEN (512+128+2) /* not including head */
int test_list_len = TEST_LIST_LEN;

#define TEST_POISON1 0xDEADBEEF
#define TEST_POISON2 0xA324354C

struct debug_el {
	unsigned int poison1;
	struct list_head list;
	unsigned int poison2;
	int value;
	unsigned serial;
};

/* Array, containing pointers to all elements in the test list */
static struct debug_el **elts __initdata;

static int __init check(struct debug_el *ela, struct debug_el *elb)
{
    if (ela->serial >= test_list_len) {
		pr_err("error: incorrect serial %d\n", ela->serial);
		return -EINVAL;
	}
    if (elb->serial >= test_list_len) {
		pr_err("error: incorrect serial %d\n", elb->serial);
		return -EINVAL;
	}
	if (elts[ela->serial] != ela || elts[elb->serial] != elb) {
		pr_err("error: phantom element\n");
		return -EINVAL;
	}
	if (ela->poison1 != TEST_POISON1 || ela->poison2 != TEST_POISON2) {
		pr_err("error: bad poison: %#x/%#x\n",
			ela->poison1, ela->poison2);
		return -EINVAL;
	}
	if (elb->poison1 != TEST_POISON1 || elb->poison2 != TEST_POISON2) {
		pr_err("error: bad poison: %#x/%#x\n",
			elb->poison1, elb->poison2);
		return -EINVAL;
	}
	return 0;
}

static int __init cmp(void *priv, struct list_head *a, struct list_head *b)
{
	struct debug_el *ela, *elb;

	ela = container_of(a, struct debug_el, list);
	elb = container_of(b, struct debug_el, list);

	check(ela, elb);
	return ela->value - elb->value;
}

static int __init list_sort_test(void)
{
	int i, count = 1, err = -ENOMEM;
	struct debug_el *el;
	struct list_head *cur;
	LIST_HEAD(head);
    int r = 1;

	pr_debug("start testing list_sort()\n");

    elts = kcalloc(test_list_len, sizeof(*elts), GFP_KERNEL);
	if (!elts)
		return err;

    printf("Random List Data[%d] ---->\n", test_list_len);

    for (i = 0; i < test_list_len; i++) {
		el = kmalloc(sizeof(*el), GFP_KERNEL);
		if (!el)
			goto exit;

        /* force some equivalencies */
        //el->value = prandom_u32() % (test_list_len / 3);
        r = (r * 725861) % 6599;
        el->value = r;
		el->serial = i;
		el->poison1 = TEST_POISON1;
		el->poison2 = TEST_POISON2;
		elts[i] = el;
		list_add_tail(&el->list, &head);

        printf("%d, ", el->value);
    }

	list_sort(NULL, &head, cmp);

    printf("\n\n");
    printf("Sorting List Data ---->\n");

    err = -EINVAL;
	for (cur = head.next; cur->next != &head; cur = cur->next) {
		struct debug_el *el1;
		int cmp_result;

		if (cur->next->prev != cur) {
			pr_err("error: list is corrupted\n");
			goto exit;
		}

		cmp_result = cmp(NULL, cur, cur->next);
		if (cmp_result > 0) {
			pr_err("error: list is not sorted\n");
			goto exit;
		}

		el = container_of(cur, struct debug_el, list);
		el1 = container_of(cur->next, struct debug_el, list);
		if (cmp_result == 0 && el->serial >= el1->serial) {
			pr_err("error: order of equivalent elements not "
				"preserved\n");
			goto exit;
		}

        printf("%d, ", el->value);

        if (check(el, el1)) {
			pr_err("error: element check failed\n");
			goto exit;
		}
		count++;
	}

	if (head.prev != cur) {
		pr_err("error: list is corrupted\n");
		goto exit;
	}

    if (count != test_list_len) {
		pr_err("error: bad list length %d", count);
		goto exit;
	}

	err = 0;
exit:
    for (i = 0; i < test_list_len; i++)
		kfree(elts[i]);
	kfree(elts);
	return err;
}

void lib_list_sort_test(void)
{
    int err;

    __fpurge(stdin);
    printf("Enter Sorting Data Counter: ");
    scanf("%d", &test_list_len);

    err = list_sort_test();

    printf("\n");
    pr_info("err = %d\n", err);
}
EXPORT_SYMBOL(lib_list_sort_test);

//module_init(list_sort_test);
//MODULE_LICENSE("GPL");
