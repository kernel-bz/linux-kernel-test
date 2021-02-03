// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/basic/bitmap-test.c
 *  Bitmap Test Module
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

#include <stdio.h>
#include <stdlib.h>

#include "test/config.h"
#include "test/debug.h"
#include "test/user-types.h"

#include <linux/bits.h>
#include <linux/bitops.h>
#include <linux/bitmap.h>
#include <asm-generic/bitops.h>
#include <uapi/linux/kernel.h>

#if 0
arch/arm/lib/bitops.h
arch/x86/boot/bitops.h
arch/**/include/asm/bitops.h
lib/bitmap.c
#endif

void bits_printf(unsigned long* v, u32 nbits)
{
    s32 i;
    u32 wc = BIT_WORD(nbits); ///include/linux/bits.h
    u64 mask1, mask2 = BIT(BITS_PER_TYPE(long) - 1); ///BITS_PER_LONG

    for (i=wc; i>=0; i--) {
        printf("v[%d]=", i);
        mask1 = mask2;
        while(mask1) {
            printf("%d", (v[i]&mask1 ? 1 : 0));
            mask1 >>= 1;
        }
        printf("\n");
    }
    printf("\n");
}

static void _bitmap_test_02(u32 nbits)
{
    pr_fn_start_on(stack_depth);

    u32 v = BITS_TO_LONGS(nbits);
    u32 len = BITS_TO_LONGS(nbits) * sizeof(unsigned long);

    pr_info_view_on(stack_depth, "%20s : %u nbits\n", nbits);
    pr_info_view_on(stack_depth, "%20s : %u array\n", v);
    pr_info_view_on(stack_depth, "%20s : %u bytes\n", len);
    pr_info_view_on(stack_depth, "%20s : %u bits\n\n", len*BITS_PER_BYTE);

    DECLARE_BITMAP(bitv, nbits);
    bits_printf(bitv, nbits);

    bitmap_zero(bitv, nbits);    ///memset(dst, 0, len)
    bits_printf(bitv, nbits);

    bitmap_fill(bitv, nbits);    ///memset(dst, 0xff, len)
    bits_printf(bitv, nbits);

    pr_fn_end_on(stack_depth);
}

static void _bitmap_test_01()
{
    s32 i;
    pr_fn_start_on(stack_depth);

    pr_info_view_on(stack_depth, "%20s : %lu bits\n", BITS_PER_TYPE(char));
    pr_info_view_on(stack_depth, "%20s : %lu bits\n", BITS_PER_TYPE(int));
    pr_info_view_on(stack_depth, "%20s : %lu bits\n", BITS_PER_TYPE(long));

    for (i=0; i<=320; i+=32) {
        pr_info_view_on(stack_depth, "%20s : %d\n", i);
        pr_info_view_on(stack_depth, "%20s : %llu\n", BITS_TO_LONGS(i));
    }

    //include/linux/types.h
    //include/linux/bitmap.h
    DECLARE_BITMAP(bitv, 300);
    pr_info_view_on(stack_depth, "%30s : %d\n",  sizeof(bitv));
    pr_info_view_on(stack_depth, "%30s : %lu\n",  sizeof(bitv) * BITS_PER_BYTE);

    pr_fn_end_on(stack_depth);
}

void bitmap_test(void)
{
    _bitmap_test_01();
    _bitmap_test_02(32);
}
