// SPDX-License-Identifier: GPL-2.0-only
/*
 *  main.c
 *  Linux Kernel Source Test Main
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
#include <linux/kernel.h>

//(DebugBase <= depth <= DebugLevel) ? debug on
int DebugBase = 0;
int DebugLevel = 200;	//S32_MAX
char DebugEnable = 1;

static void _main_menu_help(void)
{
    //help messages...
    printf("\n");
    printf("  This is the Linux kernel source testing program\n");
    printf("that can be run at the user level space.\n");
    printf("written by www.kernel.bz\n");
    printf("Tested Kernel Version: v%d.%d-td%d\n",
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
    printf("0: exit.\n");
    printf("1: Config Setting -->\n");
    printf("2: Basic Training -->\n");
    printf("3: Algorithm & Struct -->\n");
    printf("4: Start Kernel Test -->\n");
    printf("5: Scheduler Test -->\n");
    printf("6: Memory Test -->\n");
    printf("7: Drivers Test -->\n");
    printf("8: help.\n");
    printf("Kernel Version: v%d.%d-td%d\n",
           CONFIG_VERSION_1, CONFIG_VERSION_2, CONFIG_VERSION_3);
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    scanf("%d", &idx);
    return (idx > 0 && idx < asize) ? idx : -1;
}

int main(void)
{
    void (*fn[])(void) = { _main_menu_help
            , menu_config
            , menu_basic_train
            , menu_algorithm
            , menu_start_kernel
            , menu_sched_test
            , menu_mm_test
            , menu_drivers
            , _main_menu_help
    };
    int idx;

    while(1) {
        idx = _main_menu(ARRAY_SIZE(fn));
        if (idx < 0) break;
        fn[idx]();
    }

    printf("\n");
    printf("The end of kernel test. See you again... Thanks.\n\n");
    return 0;
}
