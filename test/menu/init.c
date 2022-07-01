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
#include <linux/kernel.h>
#include <linux/sched/topology.h>

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
    printf("2: numa_init test.\n");
    printf("3: sched_init test.\n");
    printf("4: rcu_init test.\n");

    printf("5: sched_init_smp test.\n");
    printf("6: sched_domain info(tl).\n");
    printf("7: sched_domain info(rq cpu).\n");
    printf("8: sched_domain info(tl cpu).\n");

    printf("9: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize - 1);
    ret = scanf("%d", &idx);
    if (ret <= 0) idx = 0;

    return (idx > 0 && idx < asize) ? idx : -1;
}

//init/main.c: start_kernel()
void menu_start_kernel(void)
{
    void (*fn[])(void) = { _init_menu_help
        , test_setup_arch
        //setup_arch(), bootmem_init()
        , test_numa_init

        , test_sched_init
        , test_rcu_init

        //rest_init(), kernel_init
        , test_sched_init_smp
        , pr_topology_info_tl
        , pr_topology_info_rq_sd_cpu
        , pr_topology_info_tl_sd_cpu

        , _init_menu_help
    };
    int idx;

    while(1) {
        idx = _init_menu(ARRAY_SIZE(fn));
        if (idx < 0) break;
        fn[idx]();
    }
}
