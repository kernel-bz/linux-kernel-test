// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/algorithm/list01.c
 *  Linked List Test 01
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stdio.h>
#include <linux/list.h>
#include "test/debug.h"

#if 0
//include/linux/stddef.h
#define offsetof(TYPE, MEMBER) ((unsigned long) &((TYPE *)0)->MEMBER)

//include/linux/kernel.h
#define container_of(ptr, type, member) ({	\
	const typeof( ((type *)0)->member ) *__mptr = (ptr); \
	(type *)( (char *)__mptr - offsetof(type, member) ); })

//include/linux/types.h
struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}
#endif

struct fox {
	unsigned long tail_length;
	unsigned long weight;
	int			  is_fantastic;
	struct list_head	list;
};

static struct fox first_fox = {
	.tail_length = 40,
	.weight = 10,
	.is_fantastic = 0,
    .list = LIST_HEAD_INIT(first_fox.list),
    //INIT_LIST_HEAD(&first_fox->list);
};

void list_test01(void)
{
    pr_fn_start_on(stack_depth);

    pr_out_on(stack_depth, "size: %d, %d, %d, %d\n",
        sizeof(struct fox), sizeof(unsigned long), sizeof(int), sizeof(struct list_head));

    pr_out_on(stack_depth, "first_fox value: %lu, %lu, %d\n"
        , first_fox.tail_length, first_fox.weight, first_fox.is_fantastic);

    pr_out_on(stack_depth, "first_fox addr: %p, %p, %p, %p, %p\n"
        , &first_fox, &first_fox.tail_length, &first_fox.weight, &first_fox.is_fantastic, &first_fox.list);

    pr_out_on(stack_depth, "offset: %d, %d, %d, %d\n"
                            , offsetof(struct fox, tail_length)
                            , offsetof(struct fox, weight)
                            , offsetof(struct fox, is_fantastic)
                            , offsetof(struct fox, list));

    pr_out_on(stack_depth, "container_of: %p, %p\n"
            , container_of(&first_fox.weight, struct fox, weight)
            , container_of(&first_fox.list, struct fox, list) );

    pr_fn_end_on(stack_depth);
}
