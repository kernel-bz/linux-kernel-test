// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/algorithm/list03.c
 *  Linked List Test 03
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

static struct fox first_fox = {
    .name = "first_fox",
    .tail_length = 11,
    .weight = 10,
    .is_fantastic = 0,
    .list = LIST_HEAD_INIT(first_fox.list),
};

static struct fox second_fox = {
    .name = "second_fox",
    .tail_length = 22,
    .weight = 20,
    .is_fantastic = 1,
    .list = LIST_HEAD_INIT(second_fox.list),
};
//list head define and init
static LIST_HEAD (fox_list);

void list_test03(void)
{
	struct list_head *p;
    struct fox *f, *fox3, *fox4;

    list_add_tail(&first_fox.list, &fox_list);
    list_add_tail(&second_fox.list, &fox_list);

    fox3 = malloc(sizeof(*fox3));
    strcpy(fox3->name,  "third_fox");
    fox3->tail_length = 33;
    fox3->weight = 30;
    fox3->is_fantastic = 0;
    INIT_LIST_HEAD(&fox3->list);

    list_add_tail(&fox3->list, &fox_list);

    fox4 = malloc(sizeof(*fox4));
    *fox4 = (struct fox) {
            .name = "fourth_fox",
            .tail_length = 44,
            .weight = 40,
            .is_fantastic = 1,
            .list = LIST_HEAD_INIT(fox4->list)
    };

    list_add_tail(&fox4->list, &fox_list);

    list_for_each(p, &fox_list) {
        f = list_entry(p, struct fox, list);
        pr_out(stack_depth, "fox value: %12s, %d, %d, %d\n"
                  , f->name, f->tail_length, f->weight, f->is_fantastic);
    }

    list_del_init(&fox_list);
    free(fox3);
    free(fox4);
}

