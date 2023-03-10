// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/algorithm/list05.c
 *  Linked List Test 05
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

static LIST_HEAD (fox_head);

void list_test05(void)
{
    struct list_head *p, *pn;
    struct fox *n, *fox;
    int i;
    char name[20];

    for (i = 1; i <= 4; i++) {
        fox = malloc(sizeof(struct fox));
        sprintf(name, "fox[%d]", i);
        strcpy(fox->name, name);
        //INIT_LIST_HEAD(&fox->list);
        list_add(&fox->list, &fox_head);
    }

    //fox[4]    fox[3]    fox[2]    fox[1]
    list_for_each(p, &fox_head) {
        fox = list_entry(p, struct fox, list);
        pr_out(stack_depth, "%s", fox->name);
    }
    pr_out(stack_depth, "\n");

    //fox[4]    fox[3]    fox[2]    fox[1]
    list_for_each_safe(p, pn, &fox_head) {
        fox = list_entry(p, struct fox, list);
        pr_out(stack_depth, "%s", fox->name);
    }
    pr_out(stack_depth, "\n");

    //fox[4]    fox[3]    fox[2]    fox[1]
    list_for_each_entry(fox, &fox_head, list)
        pr_out(stack_depth, "%s", fox->name);
    pr_out(stack_depth, "\n");

    //fox[4]    fox[3]    fox[2]    fox[1]
    list_for_each_entry_safe(fox, n, &fox_head, list)
        pr_out(stack_depth, "%s", fox->name);
    pr_out(stack_depth, "\n");

    //p = fox_head.next;
    p = fox_head.next->next;
    fox = list_entry(p, struct fox, list);

    //fox[3]    fox[2]    fox[1]
    list_for_each_entry_safe_from(fox, n, &fox_head, list)
        pr_out(stack_depth, "%s", fox->name);
    pr_out(stack_depth, "\n");

    p = fox_head.next;
    fox = list_entry(p, struct fox, list);

    //fox[3]    fox[2]    fox[1]
    list_for_each_entry_safe_continue(fox, n, &fox_head, list)
        pr_out(stack_depth, "%s", fox->name);
    pr_out(stack_depth, "\n");

    p = fox_head.next;
    fox = list_entry(p, struct fox, list);

    //head->prev
    //fox[1]    fox[2]    fox[3]    fox[4]
    list_for_each_entry_safe_reverse(fox, n, &fox_head, list)
        pr_out(stack_depth, "%s", fox->name);
    pr_out(stack_depth, "\n");

    p = fox_head.prev;
    fox = list_entry(p, struct fox, list);

    //fox[2]    fox[3]    fox[4]
    list_for_each_entry_continue_reverse(fox, &fox_head, list)
        pr_out(stack_depth, "%s", fox->name);
    pr_out(stack_depth, "\n");


    list_for_each(p, &fox_head) {
        fox = list_entry(p, struct fox, list);
        free(fox);
    }
    //list_del_init(&fox_head);
}

