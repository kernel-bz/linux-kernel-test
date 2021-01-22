
// SPDX-License-Identifier: GPL-2.0-only
/*
 *  linux/init/main.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  GK 2/5/95  -  Changed to support mounting root fs via NFS
 *  Added initrd & change_root: Werner Almesberger & Hans Lermen, Feb '96
 *  Moan early if gcc is old, avoiding bogus kernels - Paul Gortmaker, May '96
 *  Simplified starting of init:  Michael A. Griffith <grif@acm.org>
 */

#define DEBUG		/* Enable initcall_debug */

#include "test/config.h"
#include "test/debug.h"
#include "test/test.h"
#include "test/user-define.h"

#include <linux/export.h>
#include <uapi/asm-generic/setup.h>

#include <linux/types.h>
#include <linux/moduleparam.h>

#include <linux/sched/init.h>
#include <linux/init.h>
#include <asm/numa_.h>
#include <linux/cpu.h>
#include <linux/sched/clock.h>
//16 lines




//132 lines
/* Untouched command line saved by arch-specific code. */
char __initdata boot_command_line[COMMAND_LINE_SIZE];
/* Untouched saved command line (eg. for /proc) */
char *saved_command_line;
/* Command line for parameter parsing */
static char *static_command_line;
/* Command line for per-initcall parameter parsing */
static char *initcall_command_line;

static char *execute_command;
static char *ramdisk_execute_command;

/*
 * Used to generate warnings if static_key manipulation functions are used
 * before jump_label_init is called.
 */
bool static_key_initialized __read_mostly;
EXPORT_SYMBOL_GPL(static_key_initialized);
//151 lines




//453 lines
/* Check for early params. */
static int __init do_early_param(char *param, char *val,
                 const char *unused, void *arg)
{
#if 0
    const struct obs_kernel_param *p;

    for (p = __setup_start; p < __setup_end; p++) {
        if ((p->early && parameq(param, p->str)) ||
            (strcmp(param, "console") == 0 &&
             strcmp(p->str, "earlycon") == 0)
        ) {
            if (p->setup_func(val) != 0)
                pr_warn("Malformed early option '%s'\n", param);
        }
    }
#endif
    /* We accept everything at this stage. */
    return 0;
}
//472
void __init parse_early_options(char *cmdline)
{
    parse_args("early options", cmdline, NULL, 0, 0, 0, NULL,
           do_early_param);
}

/* Arch code calls this early on, or if not, just before other parsing. */
void __init parse_early_param(void)
{
    static int done __initdata;
    static char tmp_cmdline[COMMAND_LINE_SIZE] __initdata;

    if (done)
        return;

    /* All fall through to do_early_param. */
    strlcpy(tmp_cmdline, boot_command_line, COMMAND_LINE_SIZE);
    parse_early_options(tmp_cmdline);
    done = 1;
}
//493 lines


//511 lines
bool initcall_debug;



//574 lines
asmlinkage __visible void __init start_kernel(void)
{
    pr_fn_start_on(stack_depth);

    char *command_line;

    nr_cpu_ids = find_last_bit(cpumask_bits(cpu_possible_mask),NR_CPUS) + 1;

    //592 lines
    boot_cpu_init();		//kernel/cpu.c
    cpumask_setall(cpu_active_mask);
    cpumask_setall(cpu_online_mask);
    //cpumask_set_cpu(0, cpu_online_mask);
    //cpumask_clear_cpu(0, cpu_online_mask);
    cpumask_setall(cpu_present_mask);

    //596 lines
    setup_arch(&command_line);
    //setup_command_line(command_line);

    //setup_nr_cpu_ids();	//kernel/smp.c

    pr_notice("Kernel command line: %s\n", boot_command_line);
    parse_early_param();

    //638 lines
    sched_init();
    sched_clock_init();

    pr_sched_cpumask_bits_info(nr_cpu_ids);

    //arch/arm64/mm/numa.c
    numa_usr_set_node(NR_CPUS / nr_node_ids);
#ifdef CONFIG_ARM64
    arm64_numa_init();
#endif

    numa_distance_info();

    pr_fn_end_on(stack_depth);
}
