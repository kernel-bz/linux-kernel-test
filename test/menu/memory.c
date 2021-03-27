// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/menu/memory.c
 *  Memory Test Menu
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>

#include "test/config.h"
#include "test/basic.h"
#include "test/test.h"
#include <linux/kernel.h>

static void _mm_test_help(void)
{
    //help messages...
    printf("\n");
    printf("  This is the Linux kernel source testing program\n");
    printf("that can be run at the user level.\n");
    printf("This menu is to train basic kernel source.\n");
    printf("written by www.kernel.bz\n");
    printf("Tested Kernel Version: v%d.%d-td%d\n",
           CONFIG_VERSION_1, CONFIG_VERSION_2, CONFIG_VERSION_3);
    printf("\n");
    return;
}

static int _mm_test_menu(int asize)
{
    int idx, ret;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Memory Test Menu\n");
    printf("0: exit.\n");
    printf("1: memblock Test.\n");
    printf("2: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    ret = scanf("%d", &idx);
    if (ret <= 0) idx = 0;

    return (idx > 0 && idx < asize) ? idx : -1;
}

void menu_mm_test(void)
{
    void (*fn[])(void) = { _mm_test_help
        , mm_memblock_test
        , _mm_test_help
    };
    int idx;

    while(1) {
        idx = _mm_test_menu(ARRAY_SIZE(fn));
        if (idx < 0) break;
        fn[idx]();
    }
}
