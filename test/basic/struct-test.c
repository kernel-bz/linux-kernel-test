// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/basic/struct-test.c
 *  User Struct Test
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "test/debug.h"
#include "test/basic.h"

struct test_head {
    struct test_head *next, *prev;
};

struct test_set {
    int					id;
    struct test_set		*next;
    struct list_head	list;
};

typedef struct test_span { unsigned long value; } test_span_t;

struct test_struct {
    char				*name;
    unsigned long		id;
    struct test_set		**se;
    struct test_struct	*parent;
    struct test_struct	*child;
    struct test_struct	*sibling;
    test_span_t 		span[0];	//size 0
};


static int _struct_alloc(const int level, struct test_struct **ts)
{
    pr_fn_start_on(stack_depth);

    int size = level * sizeof(**ts);

    *ts = malloc(size);
    pr_view_on(stack_depth, "%30s : %d\n", level);
    pr_view_on(stack_depth, "%30s : %d\n", sizeof(struct test_struct));
    pr_view_on(stack_depth, "%30s : %d\n", sizeof(**ts));
    pr_view_on(stack_depth, "%30s : %d\n", size);
    pr_view_on(stack_depth, "%30s : %p\n", ts);

    pr_fn_end_on(stack_depth);
    return (ts) ? size : -1;
}

static void _struct_init(const int level, struct test_struct *ts)
{
    int i=0, j;

    ts[i++] = (struct test_struct){
            .name = "test0",
            .id = 0
    };

    for (j = 1; j < level; i++, j++) {
        ts[i] = (struct test_struct){
                .name = "test",
                .id = j
        };
    }

}

static int _struct_alloc_test1(const int level, struct test_struct *ts)
{
    pr_fn_start_on(stack_depth);
    int i, ret;

    ret = _struct_alloc(level, &ts);
    if (ret > 0) {
        _struct_init(level, ts);
        for (i=0; i<level; i++) {
            pr_view_on(stack_depth, "%20s : %s\n", ts[i].name);
            pr_view_on(stack_depth, "%20s : %lu\n", ts[i].id);
        }
    } else {
       pr_err("_struct_alloc_test1() error!\n");
    }

    pr_fn_end_on(stack_depth);
    return ret;
}

static void _struct_alloc_test2(const int level
                               , struct test_struct *ts, test_span_t *span)
{
    pr_fn_start_on(stack_depth);
    int i;

    span = (test_span_t *)malloc(level * sizeof(&ts->span));

    pr_view_on(stack_depth, "%20s : %d\n", level);
    //pr_view_on(stack_depth, "%20s : %d\n", sizeof(ts->span));	//0
    pr_view_on(stack_depth, "%20s : %d\n", sizeof(&ts->span));
    //pr_view_on(stack_depth, "%20s : %p\n", ts->span);	//0x30
    //pr_view_on(stack_depth, "%20s : %p\n", &ts->span);	//0x30
    pr_view_on(stack_depth, "%20s : %p\n", span);
    pr_view_on(stack_depth, "%20s : %d\n", sizeof(*ts));
    pr_view_on(stack_depth, "%20s : %d\n", sizeof(*span));
    for (i=0; i<level; i++)
        span[i].value = i;

    for (i=0; i<level; i++)
        pr_view_on(stack_depth, "%30s : %d\n", span[i].value);

    pr_fn_end_on(stack_depth);
}

static void _struct_span_value(const int level, test_span_t *span)
{
    int i;
    for (i=0; i<level; i++)
        pr_view_on(stack_depth, "%30s : %d\n", span[i].value);
}

void basic_struct_test(void)
{
    struct test_struct *ts;
    const int level = 5;
    int ret;

    ret = _struct_alloc_test1(level, ts);
    if (ret > 0) {
        _struct_alloc_test2(level, ts, &ts->span);
        //_struct_span_value(level, &ts->span);
    }
}
