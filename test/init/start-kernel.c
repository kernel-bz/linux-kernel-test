// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/init/start-kernel.c
 *  start_kernel test routine
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>

#include "test/config.h"
#include "test/debug.h"
#include "test/test.h"
#include "test/user-define.h"

#include <linux/init.h>
#include <linux/radix-tree.h>
#include <linux/sched/init.h>
#include <linux/rcupdate.h>
#include <asm/numa_.h>
#include <mm/slab.h>
#include <linux/workqueue.h>

void test_setup_arch(void)
{
    pr_fn_start_on(stack_depth);

    char *command_line;

    slab_state_set(UP);

    //init/main.c: start_kernel(): 597 lines
    setup_arch(&command_line);
    //setup_command_line(command_line);

    pr_notice("Kernel command line: %s\n", boot_command_line);
    parse_early_param();

    pr_fn_end_on(stack_depth);
}

void test_sched_init(void)
{
    nr_cpu_ids = find_last_bit(cpumask_bits(cpu_possible_mask),NR_CPUS) + 1;

    //592 lines
    boot_cpu_init();		//kernel/cpu.c
    cpumask_setall(cpu_active_mask);
    cpumask_setall(cpu_online_mask);
    //cpumask_set_cpu(0, cpu_online_mask);
    //cpumask_clear_cpu(0, cpu_online_mask);
    cpumask_setall(cpu_present_mask);

    //setup_nr_cpu_ids();	//kernel/smp.c

    //638 lines
    sched_init();
    sched_clock_init();

    //fork_init();
    //task_struct_cachep = KMEM_CACHE(task_struct, 0);
}

void test_numa_init(void)
{
     pr_sched_cpumask_bits_info(nr_cpu_ids);

    //arch/arm64/mm/numa.c
    numa_usr_set_node(NR_CPUS / nr_node_ids);
#ifdef CONFIG_ARM64
    arm64_numa_init();
#endif

    numa_distance_info();
}

void test_sched_init_smp(void)
{
    /*
     *	//init/main.c
     *	start_kernel()
     *		arch_call_rest_init()
     *			rest_init()
     * 				kernel_thread(kernel_init,..)
     * 					kernel_init()
     * 						kernel_init_freeable()
     */
    //kernel/sched/core.c
    sched_init_smp();
}

static void wq_func_test(struct work_struct *work)
{
    pr_fn_start_on(stack_depth);
    pr_view_on(stack_depth, "%20s : %p\n", work);
    pr_fn_end_on(stack_depth);
}

static DECLARE_WORK(work_test, wq_func_test);
//static DECLARE_DELAYED_WORK(work_test2, wq_func_test2);

static void wq_test(void)
{
    pr_fn_start_on(stack_depth);

    bool ret;
    ret = queue_work(system_long_wq, &work_test);
    if (!ret)
        goto _end;

    long result;
    int cpu = 0;
    result = work_on_cpu(cpu, &work_test, NULL);
    if (result < 0)
        goto _end;

    //result = schedule_on_each_cpu(&work_test);

_end:
    pr_view_on(stack_depth, "%20s : %d\n", ret);
    pr_view_on(stack_depth, "%20s : %ld\n", result);

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

void test_workqueue_init(void)
{
    radix_tree_init();

    workqueue_init_early();
    workqueue_init();

    wq_test();
}

/*
 * init/main.c
 * 	start_kernel()
 * 		rcu_init()
 *			include/linux/rcupdate.h
 * 			kernel/rcu/tiny.c
 * 			kernel/rcu/tree.c
 */
void test_rcu_init(void)
{
    kernel_rcu_init();
}
