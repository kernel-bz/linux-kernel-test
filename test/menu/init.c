// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/menu/init.c
 *  init routine menu
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>

#include "test/config.h"
#include "test/debug.h"
#include "test/test.h"

static void _init_menu_help(void)
{
    //help messages...
    printf("\n");
    printf("  This is the Linux kernel source testing program\n");
    printf("that can be run at the user level.\n");
    printf("This menu is to test kernel init sources.\n");
    printf("written by www.kernel.bz\n");
    printf("Tested Kernel Version: v%d.%d-td%d\n",
           CONFIG_VERSION_1, CONFIG_VERSION_2, CONFIG_VERSION_3);
    printf("\n");
    return;
}

static int _init_menu(int asize)
{
    int idx, ret;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Start Kernel Test Menu\n");
    printf("0: exit.\n");
    printf("1: setup_arch test.\n");
    printf("2: sched_init test.\n");
    printf("3: numa_init test.\n");
    printf("4: sched_init_smp test.\n");
    printf("5: rcu_init test.\n");
    printf("6: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    ret = scanf("%d", &idx);
    if (ret <= 0) idx = 0;

    return (idx > 0 && idx < asize) ? idx : -1;
}

//init/main.c: start_kernel()
void menu_start_kernel(void)
{
    void (*fn[])(void) = { _init_menu_help
        , test_setup_arch
        , test_sched_init
        , test_numa_init
        , test_sched_init_smp
        , test_rcu_init
        , _init_menu_help
    };
    int idx;
    int asize = sizeof (fn) / sizeof (fn[0]);

    while(1) {
        idx = _init_menu(asize);
        if (idx < 0) break;
        fn[idx]();
    }
}
