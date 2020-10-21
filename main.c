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

#include "test/basic.h"
#include "test/test.h"

static int _main_menu(int asize)
{
    int idx;
    __fpurge(stdin);

    printf("\n");
    printf("[#] Linux Kernel Source Test [Main Menu]\n");
    printf("0: help.\n");
    printf("1: basic types test.\n");
    printf("2: cpu mask test.\n");
    printf("3: Scheduler Source Test -->\n");
    printf("4: exit.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    scanf("%d", &idx);
    return (idx >= 0 && idx < asize) ? idx : -1;
}

static void _main_menu_help(void)
{
    //help messages...
    printf("\n");
    printf("It is the Linux kernel source testing program\n");
    printf("  that can be run at the user level.\n");
    printf("Written by www.kernel.bz\n");
    printf("Release Version 0.6");
    printf("\n");
    return;
}


int main(void)
{
    void (*fn[])(void) = { _main_menu_help
            , basic_types_test, cpus_mask_test, sched_test
    };
    int idx;
    int asize = sizeof (fn) / sizeof (fn[0]);

_retry:
    idx = _main_menu(asize);
    if(idx >= 0) {
        fn[idx]();
        goto _retry;
    }

    return 0;
}
