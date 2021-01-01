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

//include/linux/types.h //types-user.h
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

static struct fox red_fox = {
	.tail_length = 40,
	.weight = 10,
	.is_fantastic = 0,
	.list = LIST_HEAD_INIT (red_fox.list),
	//INIT_LIST_HEAD (&red_fox->list);
};

int list_test01(void)
{
    pr_fn_start_on(stack_depth);

    pr_out_on(stack_depth, "size: %d, %d, %d, %d\n",
        sizeof(struct fox), sizeof(unsigned long), sizeof(int), sizeof(struct list_head));

    pr_out_on(stack_depth, "red_fox value: %d, %d, %d\n"
        , red_fox.tail_length, red_fox.weight, red_fox.is_fantastic);

    pr_out_on(stack_depth, "red_fox addr: 0x%X, 0x%X, 0x%X, 0x%X, 0x%X\n"
        , &red_fox, &red_fox.tail_length, &red_fox.weight, &red_fox.is_fantastic, &red_fox.list);

    pr_out_on(stack_depth, "offset: %d, %d, %d, %d\n"
                            , offsetof(struct fox, tail_length)
                            , offsetof(struct fox, weight)
                            , offsetof(struct fox, is_fantastic)
                            , offsetof(struct fox, list));

    pr_out_on(stack_depth, "container_of: 0x%X, 0x%X\n"
            , container_of(&red_fox.weight, struct fox, weight)
            , container_of(&red_fox.list, struct fox, list) );

    pr_fn_end_on(stack_depth);
	return 0;
}
