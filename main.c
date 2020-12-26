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

#include "test/config.h"
#include "test/debug.h"
#include "test/basic.h"
#include "test/test.h"

//int DebugLevel = S32_MAX;
int DebugLevel = 200;
int DebugBase = 0;

static void _main_menu_help(void)
{
    //help messages...
    printf("\n");
    printf("Tis is the Linux kernel source testing program\n");
    printf("that can be run at the user level.\n");
    printf("written by www.kernel.bz\n");
    printf("Kernel Version: v%d.%d-td%d\n",
           CONFIG_VERSION_1, CONFIG_VERSION_2, CONFIG_VERSION_3);
    printf("\n");
    return;
}

static int _main_menu(int asize)
{
    int idx;
    __fpurge(stdin);

    printf("\n");
    printf("[*] Linux Kernel Source Test (c)www.kernel.bz\n");
    printf("0: help.\n");
    printf("1: Config Setting -->\n");
    printf("2: Basic Training -->\n");
    printf("3: Struct & Algorithm -->\n");
    printf("4: Start Kernel Test -->\n");
    printf("5: Scheduler Source Test -->\n");
    printf("6: exit.\n");
    printf("Kernel Version: v%d.%d-td%d\n",
           CONFIG_VERSION_1, CONFIG_VERSION_2, CONFIG_VERSION_3);
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    scanf("%d", &idx);
    return (idx >= 0 && idx < asize) ? idx : -1;
}

int main(void)
{
    void (*fn[])(void) = { _main_menu_help
            , config_setting
            , basic_training
            , struct_algorithm
            , init_start_kernel
            , sched_test
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
