// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/algorithm/list02.c
 *  Linked List Test 02
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/list.h>

struct fox {
	char				name[20];
	unsigned long 		tail_length;
	unsigned long 		weight;
	int			  		is_fantastic;
	struct list_head	list;
};

static struct fox red_fox = {
	.name = "red_fox",
	.tail_length = 40,
	.weight = 10,
	.is_fantastic = 0,
	.list = LIST_HEAD_INIT (red_fox.list),
	//INIT_LIST_HEAD (&red_fox->list);
};

static struct fox white_fox = {
	.name = "white_fox",
	.tail_length = 50,
	.weight = 20,
	.is_fantastic = 1,
	.list = LIST_HEAD_INIT (white_fox.list),
};
//list head define and init
static LIST_HEAD (fox_list);

int list_test02(void)
{
	struct list_head *p;
	struct fox *f;

	list_add(&red_fox.list, &fox_list);
	list_add(&white_fox.list, &fox_list);

	f = malloc(sizeof(*f));
	strcpy(f->name,  "black_fox");
	f->tail_length = 30;
	f->weight = 5;
	f->is_fantastic = 0;
	//f->list = LIST_HEAD_INIT (f->list);

	list_add(&f->list, &fox_list);

    list_for_each(p, &fox_list) {
		f = list_entry(p, struct fox, list);
		printf ("fox value: %s, %d, %d, %d\n", f->name
										, f->tail_length
										, f->weight
										, f->is_fantastic);
		printf ("\n");
	}
	return 0;
}

