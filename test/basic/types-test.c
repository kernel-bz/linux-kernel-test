// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/basic/types-test.c
 *  Data Types Test
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stdio.h>

#include "test/debug.h"
//#include "test/user-types.h"

#include <asm-generic/int-ll64.h>
#include <uapi/linux/limits.h>
#include <linux/limits.h>

static void _basic_type1_test(void)
{
    pr_fn_start(stack_depth);

    pr_view(stack_depth, "%20s : %u\n", sizeof(s8));
    pr_view(stack_depth, "%20s : %u\n", sizeof(u8));
    pr_view(stack_depth, "%20s : %u\n", sizeof(s16));
    pr_view(stack_depth, "%20s : %u\n", sizeof(u16));
    pr_view(stack_depth, "%20s : %u\n", sizeof(s32));
    pr_view(stack_depth, "%20s : %u\n", sizeof(u32));
    pr_view(stack_depth, "%20s : %u\n", sizeof(s64));
    pr_view(stack_depth, "%20s : %u\n", sizeof(u64));

    pr_fn_end(stack_depth);
}

static void _basic_type2_test(void)
{
    pr_fn_start(stack_depth);

    pr_view(stack_depth, "%20s : %u\n", U8_MAX);
    pr_view(stack_depth, "%20s : %d\n", S8_MAX);
    pr_view(stack_depth, "%20s : %d\n", S8_MIN);
    pr_view(stack_depth, "%20s : %u\n", U16_MAX);
    pr_view(stack_depth, "%20s : %d\n", S16_MAX);
    pr_view(stack_depth, "%20s : %d\n", S16_MIN);
    pr_view(stack_depth, "%20s : %u\n", U32_MAX);
    pr_view(stack_depth, "%20s : %d\n", S32_MAX);
    pr_view(stack_depth, "%20s : %d\n", S32_MIN);
    pr_view(stack_depth, "%20s : %llu\n", U64_MAX);
    pr_view(stack_depth, "%20s : %lld\n", S64_MAX);
    pr_view(stack_depth, "%20s : %lld\n", S64_MIN);

    pr_fn_end(stack_depth);
}

static void _basic_type3_test(void)
{
    pr_fn_start(stack_depth);

    pr_view(stack_depth, "%20s : %u\n", __CHAR_BIT__);
    pr_view(stack_depth, "%20s : %u\n", __SIZEOF_LONG__);
    pr_view(stack_depth, "%20s : %u\n", BITS_PER_LONG);
    pr_view(stack_depth, "%20s : %u\n", __BITS_PER_LONG);
    pr_view(stack_depth, "%20s : %u\n", BITS_PER_LONG_LONG);

    pr_fn_end(stack_depth);
}

void basic_types_test(void)
{
    printf("[#] Basic Types Test...\n");
    _basic_type1_test();
    _basic_type2_test();
    _basic_type3_test();
}
