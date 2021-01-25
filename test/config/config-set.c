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
#include <linux/xarray.h>

void config_view(void)
{
    u8 *arch[] = { "ALL", "X86", "X86_64", "ARM", "ARM64", "RISCV" };

    printf("\n");
    pr_info_view_on(stack_depth, "%30s : %s\n", arch[CONFIG_RUN_ARCH]);
    pr_info_view_on(stack_depth, "%30s : %d\n", CONFIG_HZ);
    pr_info_view_on(stack_depth, "%30s : %d\n", CONFIG_NR_CPUS);

#ifdef CONFIG_ARM64
    pr_info_view_on(stack_depth, "%30s : %s\n", "CONFIG_ARM64");
#endif
#ifdef CONFIG_NUMA
    pr_info_view_on(stack_depth, "%30s : %s\n", "CONFIG_NUMA");
#endif

    pr_info_view_on(stack_depth, "%30s : %d\n", CONFIG_BASE_SMALL);
    pr_info_view_on(stack_depth, "%30s : %d\n", XA_CHUNK_SHIFT);
    pr_info_view_on(stack_depth, "%30s : %lu\n", XA_CHUNK_SIZE);
    pr_info_view_on(stack_depth, "%30s : %d\n", CONFIG_NODES_SHIFT);

    pr_info_view_on(stack_depth, "%30s : %d\n", DebugBase);
    pr_info_view_on(stack_depth, "%30s : %d\n", DebugLevel);

    pr_info_view_on(stack_depth, "%20s : %d\n", CONFIG_VERSION_1);
    pr_info_view_on(stack_depth, "%20s : %d\n", CONFIG_VERSION_2);
    pr_info_view_on(stack_depth, "%20s : %d\n", CONFIG_VERSION_3);
}

void config_set_debug_level(void)
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
