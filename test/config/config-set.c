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

#if defined(__i386__)
#define ARCH_NAME	"x86(__i386__)"
#elif defined(__x86_64__)
#define ARCH_NAME	"x86(__x86_64__)"
#elif defined(__arm__)
#define ARCH_NAME	"arm(__arm__)"
#elif defined(__aarch64__)
#define ARCH_NAME	"arm64(__aarch64__)"
#elif defined(__riscv__)
#define ARCH_NAME	"riscv(__riscv__)"
#elif defined(__powerpc__)
#define ARCH_NAME	"(__powerpc__)"
#elif defined(__s390__)
#define ARCH_NAME	"(__s390__)"
#elif defined(__sh__)
#define ARCH_NAME	"(__sh__)"
#elif defined(__sparc__)
#define ARCH_NAME	"(__sparc__)"
#elif defined(__tile__)
#define ARCH_NAME	"(__tile__)"
#elif defined(__alpha__)
#define ARCH_NAME	"(__alpha__)"
#elif defined(__mips__)
#define ARCH_NAME	"mips(__mips__)"
#elif defined(__ia64__)
#define ARCH_NAME	"(__ia64__)"
#elif defined(__xtensa__)
#define ARCH_NAME	"(__xtensa__)"
#elif defined(__nds32__)
#define ARCH_NAME	"(__nds32__)"
#else
#define ARCH_NAME	"__unknown__"
#endif

void config_view(void)
{
    printf("\n");
    pr_view(stack_depth, "%30s : %s\n",	ARCH_NAME);
    pr_view(stack_depth, "%30s : %d\n", CONFIG_HZ);
    pr_view(stack_depth, "%30s : %d\n", CONFIG_NR_CPUS);

#ifdef CONFIG_ARM64
    pr_view(stack_depth, "%30s : %s\n", "CONFIG_ARM64");
#endif
#ifdef CONFIG_NUMA
    pr_view(stack_depth, "%30s : %s\n", "CONFIG_NUMA");
#endif

    pr_view(stack_depth, "%30s : %d\n", CONFIG_BASE_SMALL);
    pr_view(stack_depth, "%30s : %d\n", XA_CHUNK_SHIFT);
    pr_view(stack_depth, "%30s : %lu\n", XA_CHUNK_SIZE);
    pr_view(stack_depth, "%30s : %d\n", CONFIG_NODES_SHIFT);

    pr_view(stack_depth, "%30s : %s\n", CONFIG_USER_DTB_FILE);
    pr_view(stack_depth, "%30s : %s\n", dtb_file_name);
    pr_view(stack_depth, "%30s : %d\n", dtb_size);

    pr_view(stack_depth, "%30s : %d\n", DebugBase);
    pr_view(stack_depth, "%30s : %d\n", DebugLevel);
    pr_view(stack_depth, "%30s : %d\n", DebugEnable);

    pr_view(stack_depth, "%20s : %d\n", CONFIG_VERSION_1);
    pr_view(stack_depth, "%20s : %d\n", CONFIG_VERSION_2);
    pr_view(stack_depth, "%20s : %d\n", CONFIG_VERSION_3);
}

void config_set_debug(void)
{
    int dbase = DebugBase, dlevel = DebugLevel;
    int denable = (int)DebugEnable;
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

    printf("Enter Debug Enable[%d]: ", DebugEnable);
    ret = scanf("%d", &denable);
    if (ret <= 0) return;
    DebugEnable = (char)denable;
}
