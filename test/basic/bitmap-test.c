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

static void _bitmap_test_01(u32 nr)
{
    u32 i;
    pr_fn_start_on(stack_depth);

    //include/linux/bits.h
    pr_view_on(stack_depth, "%20s : %u\n", BITS_PER_BYTE);
    pr_view_on(stack_depth, "%20s : %u\n", BITS_PER_LONG);
    pr_view_on(stack_depth, "%20s : %u\n", BITS_PER_LONG_LONG);
    pr_view_on(stack_depth, "%20s : %u\n", nr);
    pr_view_on(stack_depth, "%20s : 0x%llX\n", BIT(nr));
    pr_view_on(stack_depth, "%20s : 0x%llX\n", BIT_ULL(nr));
    pr_view_on(stack_depth, "%20s : 0x%llX\n", BIT_MASK(nr));
    pr_view_on(stack_depth, "%20s : %u\n", BIT_WORD(nr));
    pr_view_on(stack_depth, "%20s : 0x%llX\n", BIT_ULL_MASK(nr));
    pr_view_on(stack_depth, "%20s : 0x%llX\n", GENMASK(nr/4, nr/8));
    pr_view_on(stack_depth, "%20s : %u\n", BIT_ULL_WORD(nr));

    //include/linux/bitops.h
    pr_view_on(stack_depth, "%30s : %u bits\n", BITS_PER_TYPE(char));
    pr_view_on(stack_depth, "%30s : %u bits\n", BITS_PER_TYPE(int));
    pr_view_on(stack_depth, "%30s : %u bits\n", BITS_PER_TYPE(long));
    pr_view_on(stack_depth, "%30s : %u bits\n", BITS_PER_TYPE(long long));

    pr_view_on(stack_depth, "%40s : %u\n", BITS_TO_BYTES(nr));
    pr_view_on(stack_depth, "%40s : %u\n", BITS_TO_U32(nr));
    pr_view_on(stack_depth, "%40s : %u\n", BITS_TO_LONGS(nr));

    pr_fn_end_on(stack_depth);
}

static void _bitmap_test_02(u32 nbits)
{
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%20s : %u\n", nbits);
    pr_view_on(stack_depth, "%20s : %u\n", BITS_TO_LONGS(nbits));

    //include/linux/types.h
    //include/linux/bitmap.h
    DECLARE_BITMAP(bitv, nbits);
    bits_printf(bitv, nbits);

    bitmap_zero(bitv, nbits);    ///memset(dst, 0, len)
    bits_printf(bitv, nbits);

    bitmap_fill(bitv, nbits);    ///memset(dst, 0xff, len)
    bits_printf(bitv, nbits);

    pr_fn_end_on(stack_depth);
}

static void _bitmap_test_03(u32 nbits)
{
    unsigned long idx0, idx1, idx2;

    pr_fn_start_on(stack_depth);

    idx2 = nbits / 2;
    idx1 = nbits / 4;
    idx0 = nbits / 8;

    DECLARE_BITMAP(bitv, nbits);
    bitmap_zero(bitv, nbits);

    //set bit index
    test_and_set_bit(idx0, bitv);
    test_and_set_bit(idx1, bitv);
    __set_bit(idx2, bitv);

    bits_printf(bitv, nbits);

    pr_view_on(stack_depth, "%30s : %u\n", idx0);
    pr_view_on(stack_depth, "%30s : %u\n", idx1);
    pr_view_on(stack_depth, "%30s : %u\n", idx2);
    pr_view_on(stack_depth, "%30s : %d\n", __bitmap_weight(bitv, nbits));

    pr_view_on(stack_depth, "%40s : %u\n", find_first_bit(bitv, nbits));
    pr_view_on(stack_depth, "%40s : %u\n", find_next_bit(bitv, nbits, idx0));
    pr_view_on(stack_depth, "%40s : %u\n", find_next_zero_bit(bitv, nbits, idx0));

    pr_view_on(stack_depth, "%40s : %u\n", test_bit(idx0 + 1, bitv));
    pr_view_on(stack_depth, "%40s : %u\n", test_bit(idx1, bitv));
    pr_view_on(stack_depth, "%40s : %u\n", test_bit(idx2, bitv));

    pr_fn_end_on(stack_depth);
}

void bitmap_test(void)
{
    u32 nr;

    __fpurge(stdin);
    printf("Input bit number: ");
    scanf("%u", &nr);

    _bitmap_test_01(nr);
    _bitmap_test_02(nr);
    _bitmap_test_03(nr);
}
