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

#include <linux/limits.h>
#include <linux/sched/clock.h>
#include <linux/sched/init.h>

#include "test/config.h"
#include "test/debug.h"
#include "test/basic.h"
#include "test/test.h"

int DebugLevel = S32_MAX;
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

static int _main_start_kernel_menu(int asize)
{
    int idx;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Start Kernel Test Menu\n");
    printf("0: help.\n");
    printf("1: sched_init test.\n");
    printf("2: sched_init_smp test.\n");
    printf("3: exit.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    scanf("%d", &idx);
    return (idx >= 0 && idx < asize) ? idx : -1;
}

//init/main.c:
//  start_kernel()
//    rest_init()
//      kernel_thread(kernel_init, NULL, CLONE_FS)
//        kernel_init()
//          kernel_init_freeable()
static void _main_start_kernel(void)
{
    sched_init();
    sched_clock_init();
}

void _main_start_kernel_test(void)
{
    void (*fn[])(void) = { _main_menu_help
        , _main_start_kernel
        , sched_init_smp	//kernel/sched/core.c
    };
    int idx;
    int asize = sizeof (fn) / sizeof (fn[0]);

_retry:
    idx = _main_start_kernel_menu(asize);
    if (idx >= 0) {
        fn[idx]();
        goto _retry;
    }
}

static int _main_menu(int asize)
{
    int idx;
    __fpurge(stdin);

    printf("\n");
    printf("[*] Linux Kernel Source Test (c)www.kernel.bz\n");
    printf("0: help.\n");
    printf("1: basic types test.\n");
    printf("2: cpu mask test.\n");
    printf("3: Start Kernel Test -->\n");
    printf("4: Scheduler Source Test -->\n");
    printf("5: exit.\n");
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
            , basic_types_test, cpus_mask_test
            , _main_start_kernel_test
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
