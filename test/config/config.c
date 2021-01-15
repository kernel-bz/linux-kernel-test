// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/config/config.c
 *  Config Setting
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>

#include "test/debug.h"
#include "test/config.h"

static void _config_view(void)
{
    printf("\n");
    pr_info_view_on(stack_depth, "%20s : %d\n", CONFIG_HZ);
#ifdef CONFIG_ARM64
    pr_info_view_on(stack_depth, "%20s : %s\n", "CONFIG_ARM64");
#endif
    pr_info_view_on(stack_depth, "%20s : %d\n", CONFIG_NR_CPUS);
#ifdef CONFIG_NUMA
    pr_info_view_on(stack_depth, "%20s : %s\n", "CONFIG_NUMA");
#endif
    pr_info_view_on(stack_depth, "%20s : %d\n", CONFIG_NODES_SHIFT);

    pr_info_view_on(stack_depth, "%20s : %d\n", DebugBase);
    pr_info_view_on(stack_depth, "%20s : %d\n", DebugLevel);
}

static void _config_set_debug_level(void)
{
    int dbase, dlevel;

    __fpurge(stdin);
    printf("Enter Debug Base Number[0,%d]: ", DebugBase);
    scanf("%d", &dbase);
    printf("Enter Debug Level Number[0,%d]: ", DebugLevel);
    scanf("%d", &dlevel);

    DebugLevel = dlevel;
    DebugBase = dbase;
}

static void _config_setting_help(void)
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

static int _config_setting_menu(int asize)
{
    int idx;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Config Setting Menu\n");
    printf("0: exit.\n");
    printf("1: Config View.\n");
    printf("2: Set Debug Level.\n");
    printf("3: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    scanf("%d", &idx);
    return (idx > 0 && idx < asize) ? idx : -1;
}

void config_setting(void)
{
    void (*fn[])(void) = { _config_setting_help
        , _config_view
        , _config_set_debug_level
        , _config_setting_help
    };
    int idx;
    int asize = sizeof (fn) / sizeof (fn[0]);

    while(1) {
        idx = _config_setting_menu(asize);
        if (idx < 0) break;
        fn[idx]();
    }
}
