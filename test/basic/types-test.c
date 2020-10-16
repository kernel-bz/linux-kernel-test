// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/basic/types-test.c
 *  Data Types Test
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stdio.h>
#include <stdlib.h>

#include "type01.h"
#include "type02.h"
#include "type03.h"
#include "type-limits.h"

static void _basic_type01_test(void)
{
    printf("s8_t = %u bytes\n", sizeof(s8_t));
    printf("u8_t = %u bytes\n", sizeof(u8_t));
    printf("s16_t = %u bytes\n", sizeof(s16_t));
    printf("u16_t = %u bytes\n", sizeof(u16_t));
    printf("s32_t = %u bytes\n", sizeof(s32_t));
    printf("u32_t = %u bytes\n", sizeof(u32_t));
    printf("s64_t = %u bytes\n", sizeof(s64_t));
    printf("u64_t = %u bytes\n", sizeof(u64_t));
    printf("_s64_t = %u bytes\n", sizeof(_s64_t));
    printf("_u64_t = %u bytes\n", sizeof(_u64_t));
    printf("\n");
}

static void _basic_type02_test(void)
{
    printf("s8 = %u bytes\n", sizeof(s8));
    printf("u8 = %u bytes\n", sizeof(u8));
    printf("s16 = %u bytes\n", sizeof(s16));
    printf("u16 = %u bytes\n", sizeof(u16));
    printf("s32 = %u bytes\n", sizeof(s32));
    printf("u32 = %u bytes\n", sizeof(u32));
    printf("s64 = %u bytes\n", sizeof(s64));
    printf("u64 = %u bytes\n", sizeof(u64));
    printf("\n");
}

static void _basic_type03_test(void)
{
    printf("U8_MAX=%u\n", U8_MAX);
    printf("S8_MAX=%d\n", S8_MAX);
    printf("S8_MIN=%d\n", S8_MIN);
    printf("U16_MAX=%u\n", U16_MAX);
    printf("S16_MAX=%d\n", S16_MAX);
    printf("S16_MIN=%d\n", S16_MIN);
    printf("U32_MAX=%u\n", U32_MAX);
    printf("S32_MAX=%d\n", S32_MAX);
    printf("S32_MIN=%d\n", S32_MIN);
    printf("U64_MAX=%llu\n", U64_MAX);
    printf("S64_MAX=%lld\n", S64_MAX);
    printf("S64_MIN=%lld\n", S64_MIN);
    printf("\n");
}

void basic_types_test(void)
{
    _basic_type01_test();
    _basic_type02_test();
    _basic_type03_test();
}
