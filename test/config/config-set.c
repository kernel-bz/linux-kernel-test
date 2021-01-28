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
#include "test/dtb-test.h"
#include <linux/xarray.h>

void config_view(void)
{
    u8 *arch[] = { "ALL", "X86", "X86_64", "ARM", "ARM64", "RISCV" };

    printf("\n");
    pr_out_view(stack_depth, "%30s : %s\n", arch[CONFIG_RUN_ARCH]);
    pr_out_view(stack_depth, "%30s : %d\n", CONFIG_HZ);
    pr_out_view(stack_depth, "%30s : %d\n", CONFIG_NR_CPUS);

#ifdef CONFIG_ARM64
    pr_out_view(stack_depth, "%30s : %s\n", "CONFIG_ARM64");
#endif
#ifdef CONFIG_NUMA
    pr_out_view(stack_depth, "%30s : %s\n", "CONFIG_NUMA");
#endif

    pr_out_view(stack_depth, "%30s : %d\n", CONFIG_BASE_SMALL);
    pr_out_view(stack_depth, "%30s : %d\n", XA_CHUNK_SHIFT);
    pr_out_view(stack_depth, "%30s : %lu\n", XA_CHUNK_SIZE);
    pr_out_view(stack_depth, "%30s : %d\n", CONFIG_NODES_SHIFT);

    pr_out_view(stack_depth, "%30s : %s\n", CONFIG_USER_DTB_FILE);
    pr_out_view(stack_depth, "%30s : %s\n", dtb_file_name);
    pr_out_view(stack_depth, "%30s : %d\n", dtb_size);

    pr_out_view(stack_depth, "%30s : %d\n", DebugBase);
    pr_out_view(stack_depth, "%30s : %d\n", DebugLevel);

    pr_out_view(stack_depth, "%20s : %d\n", CONFIG_VERSION_1);
    pr_out_view(stack_depth, "%20s : %d\n", CONFIG_VERSION_2);
    pr_out_view(stack_depth, "%20s : %d\n", CONFIG_VERSION_3);
}

void config_set_debug_level(void)
{
    int dbase=DebugBase, dlevel=DebugLevel;
    int ret;

    __fpurge(stdin);

    printf("Enter Debug Base Number[%d]: ", DebugBase);
    ret = scanf("%d", &dbase);
    if (ret <= 0) return;
    DebugBase = dbase;

    printf("Enter Debug Level Number[%d]: ", DebugLevel);
    ret = scanf("%d", &dlevel);
    if (ret <= 0) return;
    DebugLevel = dlevel;
}
