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

struct _list_head {
    struct _list_head *next, *prev;
};

struct test_set {
    int					id;
    struct test_set		*next;
    struct _list_head	list;
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

struct test_class {
    char a;
    int b;
    long c;
};

//DEFINE_SCHED_CLASS
#define DEFINE_STRUCT_TEST(name) \
const struct test_class name##_class \
        __aligned(__alignof__(struct test_class)) \
        __section(#name)

DEFINE_STRUCT_TEST(test1) = {
        .a = 10,
        .b = 100,
        .c = 1000
};

DEFINE_STRUCT_TEST(test2) = {
        .a = 20,
        .b = 200,
        .c = 2000
};

DEFINE_STRUCT_TEST(test3) = {
        .a = 30,
        .b = 300,
        .c = 3000
};

#define for_test_class_range(class, _from, _to) \
        for (class = (_from); class <= (_to); class++)


#define DEFINE_STRUCT_ALIGN(name, align) \
const struct test_class name##_class \
        __aligned(align) __section(#name)

DEFINE_STRUCT_ALIGN(align4_1, 16);		//1 << 4
DEFINE_STRUCT_ALIGN(align4_2, 16);
DEFINE_STRUCT_ALIGN(align4_3, 16);
DEFINE_STRUCT_ALIGN(align8_1, 256);		//1 << 8
DEFINE_STRUCT_ALIGN(align8_2, 256);
DEFINE_STRUCT_ALIGN(align8_3, 256);
DEFINE_STRUCT_ALIGN(align12_1, 1 << 12);
DEFINE_STRUCT_ALIGN(align12_2, 1 << 12);
DEFINE_STRUCT_ALIGN(align12_3, 1 << 12);

void basic_struct_test2(void)
{
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%30s : %d\n", sizeof(test1_class));
    pr_view_on(stack_depth, "%30s : %d\n", __alignof__(struct test_class));

    pr_view_on(stack_depth, "%20s : %p\n", &test1_class);
    pr_view_on(stack_depth, "%20s : %p\n", &test2_class);
    pr_view_on(stack_depth, "%20s : %p\n", &test3_class);

    struct test_class *test_class;
    for_test_class_range(test_class, &test1_class, &test3_class)
            pr_view_on(stack_depth, "%30s : %p\n", test_class);

    pr_view_on(stack_depth, "%30s : %d\n", &test3_class - &test1_class);

    pr_out_on(stack_depth, "\n");
    pr_view_on(stack_depth, "%30s : %d\n", sizeof(align4_1_class));
    pr_view_on(stack_depth, "%30s : %d\n", __alignof__(align4_1_class));
    pr_view_on(stack_depth, "%20s : %p\n", &align4_1_class);
    pr_view_on(stack_depth, "%20s : %p\n", &align4_2_class);
    pr_view_on(stack_depth, "%20s : %p\n", &align4_3_class);
    pr_view_on(stack_depth, "%30s : %d\n\n", &align4_3_class - &align4_1_class);

    pr_view_on(stack_depth, "%30s : %d\n", sizeof(align8_1_class));
    pr_view_on(stack_depth, "%30s : %d\n", __alignof__(align8_1_class));
    pr_view_on(stack_depth, "%20s : %p\n", &align8_1_class);
    pr_view_on(stack_depth, "%20s : %p\n", &align8_2_class);
    pr_view_on(stack_depth, "%20s : %p\n", &align8_3_class);
    pr_view_on(stack_depth, "%30s : %d\n\n", &align8_3_class - &align8_1_class);

    pr_view_on(stack_depth, "%30s : %d\n", sizeof(align12_1_class));
    pr_view_on(stack_depth, "%30s : %d\n", __alignof__(align12_1_class));
    pr_view_on(stack_depth, "%20s : %p\n", &align12_1_class);
    pr_view_on(stack_depth, "%20s : %p\n", &align12_2_class);
    pr_view_on(stack_depth, "%20s : %p\n", &align12_3_class);
    pr_view_on(stack_depth, "%30s : %d\n", &align12_3_class - &align12_1_class);

    pr_fn_end_on(stack_depth);
}

