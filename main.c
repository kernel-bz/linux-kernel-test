// SPDX-License-Identifier: GPL-2.0-only
/*
 *  main.c
 *  Linux Kernel Source(v5.4) Test Main
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "test/test.h"

static int _main_menu(void)
{
    printf("\n");
    printf("********** Test Menu (www.kernel.bz) **********\n");
    printf("0: help.\n");
    printf("1: cpu mask test.\n");
    printf("2: ++ Schedular Source Test ++\n");
    printf("other: exit.\n");
    printf("\n");
    return 0;
}


int main(void)
{
    int (*fn[])(void) = { _main_menu
                , cpus_mask_test, sched_test
        };
    int idx = -1;
    int asize = sizeof (fn) / sizeof (fn[0]);

_retry:
    __fpurge(stdin);
    _main_menu();
    printf("Enter Menu Number[0,%d]: ", asize);
    scanf("%d", &idx);
    if(idx >= 0 && idx < asize) {
        fn[idx]();
        goto _retry;
    }

    return 0;
}
