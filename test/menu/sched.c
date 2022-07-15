// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/menu/sched.c
 *  Linux Kernel Schedular Test Menu
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test/test.h"
#include "test/debug.h"
#include <linux/kernel.h>

static void _sched_test_help(void)
{
    //help messages...
    printf("\n");
    printf("  This menu is to test Linux kernel scheduler sources\n");
    printf("at the user level as follows:\n");
    printf("written by www.kernel.bz\n");
    printf("Tested Kernel Version: v%d.%d-td%d\n",
           CONFIG_VERSION_1, CONFIG_VERSION_2, CONFIG_VERSION_3);
    printf("\n");
    return;
}

static int _sched_stop_test_menu(int asize)
{
    int idx, ret;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Scheduler --> Stop Test Menu\n");
    printf("0: exit.\n");
    printf("1: sched_cpu_dying test.\n");

    printf("2: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize - 1);
    ret = scanf("%d", &idx);
    if (ret <= 0) idx = 0;

    return (idx > 0 && idx < asize) ? idx : -1;
}

static void _sched_stop_test(void)
{
    void (*fn[])(void) = { _sched_test_help
        , test_sched_stop_cpu_dying
        , _sched_test_help
    };
    int idx;

    while (1) {
        idx = _sched_stop_test_menu(ARRAY_SIZE(fn));
        if (idx < 0) break;
        fn[idx]();
    }
}

static int _sched_dl_test_menu(int asize)
{
    int idx, ret;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Scheduler --> DeadLine Test Menu\n");
    printf("0: exit.\n");
    printf("1: deadline enqueue test.\n");
    printf("2: cpudl test.\n");
    printf("3: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize - 1);
    ret = scanf("%d", &idx);
    if (ret <= 0) idx = 0;

    return (idx > 0 && idx < asize) ? idx : -1;
}

static void _sched_dl_test(void)
{
    void (*fn[])(void) = { _sched_test_help
        , test_sched_dl_enqueue
        , test_sched_cpudl
        , _sched_test_help
    };
    int idx;

    while (1) {
        idx = _sched_dl_test_menu(ARRAY_SIZE(fn));
        if (idx < 0) break;
        fn[idx]();
    }
}

static int _sched_rt_test_menu(int asize)
{
    int idx, ret;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Scheduler --> RT Test Menu\n");
    printf("0: exit.\n");
    printf("1: rt_rq info.\n");
    printf("2: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize - 1);
    ret = scanf("%d", &idx);
    if (ret <= 0) idx = 0;

    return (idx > 0 && idx < asize) ? idx : -1;
}

static void _sched_rt_test(void)
{
    void (*fn[])(void) = { _sched_test_help
           , rt_debug_rt_rq_info
           , _sched_test_help
    };
    int idx;

    while (1) {
        idx = _sched_rt_test_menu(ARRAY_SIZE(fn));
        if (idx < 0) break;
        fn[idx]();
    }
}

static int _sched_pelt_test_menu(int asize)
{
    int idx, ret;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Scheduler --> CFS --> PELT Test Menu\n");
    printf("0: exit.\n");
    printf("1: sched_pelt_constants() create.\n");
    printf("2: calc_global_load test.\n");
    printf("3: decay_load() test.\n");
    printf("4: update_load_avg() test.\n");
    printf("5: sched pelt info.\n");
    printf("6: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize - 1);
    ret = scanf("%d", &idx);
    if (ret <= 0) idx = 0;

    return (idx > 0 && idx < asize) ? idx : -1;
}

static void _sched_pelt_test(void)
{
    void (*fn[])(void) = { _sched_test_help
        , sched_pelt_constants
        , test_calc_global_load
        , test_decay_load
        , test_update_load_avg
        , test_sched_pelt_info
        , _sched_test_help
    };
    int idx;

_retry:
    while (1) {
        idx = _sched_pelt_test_menu(ARRAY_SIZE(fn));
        if (idx < 0) break;
        fn[idx]();
    }
}

static int _sched_cfs_test_menu(int asize)
{
    int idx, ret;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Scheduler --> CFS Test Menu\n");
    printf("0: exit.\n");

    printf("1: set_user_nice test.\n");
    printf("2: run rebalance test.\n");
    printf("3: vruntime __calc_delta test.\n");
    printf("4: leaf_cfs_rq_list info.\n");
    printf("5: cfs_tasks info.\n");
    printf("6: PELT test -->\n");

    printf("7: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize - 1);
    ret = scanf("%d", &idx);
    if (ret <= 0) idx = 0;

    return (idx > 0 && idx < asize) ? idx : -1;
}

static void _sched_cfs_test(void)
{
    void (*fn[])(void) = { _sched_test_help
        , test_sched_set_user_nice
        , sched_fair_run_rebalance	//kernel/sched/fair.c
        , sched_fair_vruntime_test	//kernel/sched/fair.c
        , pr_leaf_cfs_rq_info
        , pr_sched_cfs_tasks_info
        , _sched_pelt_test

        , _sched_test_help
    };
    int idx;

    while (1) {
        idx = _sched_cfs_test_menu(ARRAY_SIZE(fn));
        if (idx < 0) break;
        fn[idx]();
    }
}

static int _sched_idle_test_menu(int asize)
{
    int idx, ret;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Scheduler --> Idle Test Menu\n");
    printf("0: exit.\n");
    printf("1: deadline enqueue test.\n");
    printf("2: cpudl test.\n");
    printf("3: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize - 1);
    ret = scanf("%d", &idx);
    if (ret <= 0) idx = 0;

    return (idx > 0 && idx < asize) ? idx : -1;
}

static void _sched_idle_test(void)
{
    void (*fn[])(void) = { _sched_test_help
        , test_sched_dl_enqueue
        , test_sched_cpudl
        , _sched_test_help
    };
    int idx;

    while (1) {
        idx = _sched_idle_test_menu(ARRAY_SIZE(fn));
        if (idx < 0) break;
        fn[idx]();
    }
}

static int _sched_test_menu(int asize)
{
    int idx, ret;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Scheduler Source Test Menu\n");
    printf(" 0: exit.\n");

    printf(" 1: create task group test.\n");
    printf(" 2: task group info.\n");
    printf(" 3: task group info(detail).\n");

    printf(" 4: wake_up_new_task test.\n");
    printf(" 5: setscheduler test.\n");
    printf(" 6: schedule test.\n");
    printf(" 7: wake up process test.\n");
    printf(" 8: deactivate_task test.\n");

    printf(" 9: current task info.\n");
    printf("10: cpu rq info.\n");

    printf("11: Stop Scheduler Test -->\n");
    printf("12: DeadLine Scheduler Test -->\n");
    printf("13: RT Scheduler Test -->\n");
    printf("14: CFS Scheduler Test -->\n");
    printf("15: Idle Scheduler Test -->\n");

    printf("15: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize - 1);
    ret = scanf("%d", &idx);
    if (ret <= 0) idx = 0;

    return (idx > 0 && idx < asize) ? idx : -1;
}

void menu_sched_test(void)
{
    void (*fn[])(void) = { _sched_test_help
        , test_sched_create_group
        , pr_sched_tg_info
        , pr_sched_tg_info_all

        , test_sched_new_task
        , test_sched_setscheduler
        , test_sched_schedule
        , test_sched_wake_up_process
        , test_sched_deactivate_task

        , test_sched_current_task_info
        , test_sched_rq_info

        , _sched_stop_test
        , _sched_dl_test
        , _sched_rt_test
        , _sched_cfs_test
        , _sched_idle_test

        , _sched_test_help
    };
    int idx;

    while(1) {
        idx = _sched_test_menu(ARRAY_SIZE(fn));
        if (idx < 0) break;
        fn[idx]();
    }
}
