// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/algorithm/list04.c
 *  Linked List Test 04
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/list.h>
#include "test/debug.h"

static struct fox {
	char				name[20];
	unsigned long 		tail_length;
	unsigned long 		weight;
	int			  		is_fantastic;
	struct list_head	list;
};

//list head define and init
static LIST_HEAD (ListHeadFox);

static struct fox* fox_alloc(char *name, unsigned long tail, unsigned long weight, int isf)
{
	struct fox* fox;

	fox = malloc(sizeof(*fox));
	if (!fox) return NULL;

	strcpy(fox->name, name);
	fox->tail_length = tail;
	fox->weight = weight;
	fox->is_fantastic = isf;
    INIT_LIST_HEAD(&fox->list);

	return fox;
}

static void list_free(struct list_head *head)
{
    pr_fn_start(stack_depth);

    struct list_head *list_head;
    struct fox* fox;

    list_for_each(list_head, head) {
        fox = list_entry(list_head, struct fox, list);
        pr_view(stack_depth, "%20s : %s\n", fox->name);
        list_del(head);
        free(fox);
    }

    pr_fn_end(stack_depth);
}

static void list_output(struct list_head *head)
{
    pr_fn_start(stack_depth);

	struct list_head *list_head;
	struct fox* fox;

    list_for_each(list_head, head) {
		fox = list_entry(list_head, struct fox, list);
        pr_out(stack_depth, "fox value: %12s, %d, %d, %d\n"
                  , fox->name, fox->tail_length, fox->weight, fox->is_fantastic);
	}

    pr_fn_end(stack_depth);
}

static void fox_alloc_run(void)
{
    struct fox* fox;

    fox = fox_alloc("first_fox", 11, 10, 0);
    if (!fox) return -1;
    list_add(&fox->list, &ListHeadFox);

    fox = fox_alloc("second_fox", 22, 20, 1);
    if (!fox) return -1;
    list_add(&fox->list, &ListHeadFox);

    fox = fox_alloc("third_fox", 33, 30, 2);
    if (!fox) return -1;
    list_add(&fox->list, &ListHeadFox);

    fox = fox_alloc("fourth_fox", 44, 40, 3);
    if (!fox) return -1;
    list_add(&fox->list, &ListHeadFox);
}

void list_test04(void)
{
	LIST_HEAD (list_head_fox2);
	LIST_HEAD (list_head_fox3);

    fox_alloc_run();
	list_output(&ListHeadFox);

    //cut to list_head_fox2 from ListHeadFox
    list_cut_position(&list_head_fox2, &ListHeadFox, (&ListHeadFox)->next->next);
	list_output(&ListHeadFox);
    list_output(&list_head_fox2);

    //merge ListHeadFox to list_head_fox3
    list_splice(&ListHeadFox, &list_head_fox3);
    //merge list_head_fox2 to list_head_fox3
    list_splice(&list_head_fox2, &list_head_fox3);

    //list_output(&ListHeadFox);	//infinite
    if (list_empty(&ListHeadFox)) {
        pr_view(stack_depth, "%20s : empty: %p\n", &ListHeadFox);
    }
    //list_output(&list_head_fox2);	//infinite
    if (list_empty(&list_head_fox2)) {
        pr_view(stack_depth, "%20s : empty: %p\n", &list_head_fox2);
    }
    list_output(&list_head_fox3);

    while (!list_empty(&ListHeadFox))
            list_del_init(&ListHeadFox);
    while (!list_empty(&list_head_fox2))
            list_del_init(&list_head_fox2);
    while (!list_empty(&list_head_fox3))
            list_del_init(&list_head_fox3);
}
