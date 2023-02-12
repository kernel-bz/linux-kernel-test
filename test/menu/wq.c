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

    pr_view_on(stack_depth, "%30s : %d\n", WORK_STRUCT_COLOR_SHIFT);
    pr_view_on(stack_depth, "%30s : %d\n", WORK_STRUCT_COLOR_BITS);
    pr_view_on(stack_depth, "%30s : %d\n", WORK_NR_COLORS);
    pr_view_on(stack_depth, "%30s : %d\n", WORK_STRUCT_FLAG_BITS);
    pr_view_on(stack_depth, "%30s : %d\n", WORK_CPU_UNBOUND);
    pr_view_on(stack_depth, "%30s : %d\n", NR_CPUS);
    pr_out_on(stack_depth, "\n");
    pr_view_on(stack_depth, "%30s : %d\n", WORK_OFFQ_FLAG_BASE);
    pr_view_on(stack_depth, "%30s : %d\n", WORK_OFFQ_CANCELING);
    pr_view_on(stack_depth, "%30s : %d\n", WORK_OFFQ_POOL_SHIFT);
    pr_view_on(stack_depth, "%30s : %d\n", WORK_OFFQ_LEFT);
    pr_view_on(stack_depth, "%30s : %d\n", WORK_OFFQ_POOL_BITS);
    pr_view_on(stack_depth, "%30s : %d\n", WORK_OFFQ_POOL_NONE);
    pr_view_on(stack_depth, "%30s : %d\n", INT_MAX);

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
static DECLARE_WORK(test_work1, _wq_func);
static DECLARE_WORK(test_work2, _wq_func);
static DECLARE_WORK(test_work3, _wq_func);
static DECLARE_WORK(test_work4, _wq_func);
static DECLARE_WORK(test_work5, _wq_func);
static DECLARE_WORK(test_work6, _wq_func);

static void _wq_queue_work_test(void)
{
    const struct workqueue_struct *wq[7] = {
        system_wq, system_highpri_wq, system_long_wq, system_unbound_wq
      , system_freezable_wq, system_power_efficient_wq, system_freezable_power_efficient_wq
    };
    const struct work_struct *work[7] = {
                  &test_work, &test_work1, &test_work2, &test_work3
                , &test_work4, &test_work5, &test_work6
    };
    int idx, cpu;

    pr_fn_start_on(stack_depth);

_retry:
    __fpurge(stdin);
    printf("0: system_wq\n");
    printf("1: system_highpri_wq\n");
    printf("2: system_long_wq\n");
    printf("3: system_unbound_wq\n");
    printf("4: system_freezable_wq\n");
    printf("5: system_power_efficient_wq\n");
    printf("6: system_freezable_power_efficient_wq\n\n");

    printf("Enter workqueue_struct Index Number[0, 6]: ");
    scanf("%d", &idx);
    if (idx < 0 || idx > 6)
        goto _retry;

_retry_cpu:
    __fpurge(stdin);
    printf("Enter CPU Number[0, %d]: ", nr_cpu_ids);
    scanf("%d", &cpu);
    if (cpu < 0 || cpu > nr_cpu_ids)
        goto _retry_cpu;

    if (cpu >= nr_cpu_ids) {
        //cpu = raw_smp_processor_id()
        queue_work(wq[idx], work[idx]);
    } else {
        //system_wq
        work_on_cpu(cpu, _wq_func, work[idx]);
        //schedule_on_each_cpu(&test_work);
    }

    pr_fn_end_on(stack_depth);
}

static void _wq_wake_up_worker_test(void)
{
    int id;

    __fpurge(stdin);
    printf("Enter worker_pool id: ");
    scanf("%d", &id);
    if (id < 0)
        return;

    wake_up_worker_run(id);
}

static int _wq_test_menu(int asize)
{
    int idx, ret;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> WorkQueue Test Menu\n");
    printf("0: exit.\n");
    printf("1: show wq constants info.\n");
    printf("2: queue_work() test.\n");
    printf("3: show queue/insert work result.\n");
    printf("4: show_all_pool_info().\n");
    printf("5: wake_up_worker_test().\n");
    printf("6: show_all_workqueues_info().\n");
    printf("7: show_all_unbound_info().\n");
    printf("8: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize - 1);
    ret = scanf("%d", &idx);
    if (ret <= 0) idx = 0;

    return (idx > 0 && idx < asize) ? idx : -1;
}

void menu_wq_test(void)
{
    void (*fn[])(void) = { _wq_test_help
        , _wq_constant_info
        , _wq_queue_work_test
        , show_insert_work_result
        , show_all_pool_info
        , _wq_wake_up_worker_test
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
