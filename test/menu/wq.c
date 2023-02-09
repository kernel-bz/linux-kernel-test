// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/menu/wq.c
 *  WorkQueue Test Menu
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>

#include "test/config.h"
#include "test/debug.h"
#include "test/basic.h"
#include "test/test.h"

#include <linux/kernel.h>
#include <linux/workqueue.h>
#include "kernel/workqueue_internal.h"

static void _wq_test_help(void)
{
    //help messages...
    printf("\n");
    printf("  This is the Linux kernel source testing program\n");
    printf("that can be run at the user level.\n");
    printf("This menu is to train basic kernel source.\n");
    printf("written by www.kernel.bz\n");
    printf("Tested Kernel Version: v%d.%d-td%d\n",
           CONFIG_VERSION_1, CONFIG_VERSION_2, CONFIG_VERSION_3);
    printf("\n");
    return;
}

static void _wq_constant_info(void)
{
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%20s : %d\n", WORK_NR_COLORS);
    pr_view_on(stack_depth, "%20s : %d\n", WORK_OFFQ_FLAG_BASE);
    pr_view_on(stack_depth, "%20s : %d\n", WORK_OFFQ_CANCELING);
    pr_view_on(stack_depth, "%20s : %d\n", WORK_OFFQ_POOL_SHIFT);
    pr_view_on(stack_depth, "%20s : %d\n", WORK_OFFQ_LEFT);
    pr_view_on(stack_depth, "%20s : %d\n", WORK_OFFQ_POOL_BITS);
    pr_view_on(stack_depth, "%20s : %d\n", WORK_OFFQ_POOL_NONE);
    pr_view_on(stack_depth, "%20s : %d\n", INT_MAX);

    pr_view_on(stack_depth, "%30s : 0x%llX\n", WORK_STRUCT_NO_POOL);

    pr_view_on(stack_depth, "%30s : 0x%llX\n", WORK_STRUCT_FLAG_BITS);
    pr_view_on(stack_depth, "%30s : 0x%llX\n", WORK_STRUCT_FLAG_MASK);
    pr_view_on(stack_depth, "%30s : 0x%llX\n", WORK_STRUCT_WQ_DATA_MASK);

    pr_fn_end_on(stack_depth);
}

static void _wq_func(struct work_struct *work)
{
    pr_fn_start_on(stack_depth);
    pr_view_on(stack_depth, "%20s : %p\n", work);
    pr_fn_end_on(stack_depth);
}

static DECLARE_WORK(test_work, _wq_func);
//static DECLARE_DELAYED_WORK(test_work2, _wq_func);


static void _wq_queue_work_test(void)
{
    pr_fn_start_on(stack_depth);

    bool ret;
    ret = queue_work(system_long_wq, &test_work);
    if (!ret)
        goto _end;

    long result;
    int cpu = 0;
    result = work_on_cpu(cpu, &test_work, NULL);
    if (result < 0)
        goto _end;

    //result = schedule_on_each_cpu(&test_work);

_end:
    pr_view_on(stack_depth, "%20s : %d\n", ret);
    pr_view_on(stack_depth, "%20s : %ld\n", result);

    pr_fn_end_on(stack_depth);
}

static int _wq_test_menu(int asize)
{
    int idx, ret;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> WorkQueue Test Menu\n");
    printf("0: exit.\n");
    printf("1: queue_work() test.\n");
    printf("2: show queue/insert work result.\n");
    printf("3: show wq constants info.\n");
    printf("4: show_all_pool_info().\n");
    printf("5: show_all_workqueues_info().\n");
    printf("6: show_all_unbound_info().\n");
    printf("7: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize - 1);
    ret = scanf("%d", &idx);
    if (ret <= 0) idx = 0;

    return (idx > 0 && idx < asize) ? idx : -1;
}

void menu_wq_test(void)
{
    void (*fn[])(void) = { _wq_test_help
        , _wq_queue_work_test
        , show_insert_work_result
        , _wq_constant_info
        , show_all_pool_info
        , show_all_workqueues_info
        , show_all_unbound_info
        , _wq_test_help
    };
    int idx;

    while(1) {
        idx = _wq_test_menu(ARRAY_SIZE(fn));
        if (idx < 0) break;
        fn[idx]();
    }
}
