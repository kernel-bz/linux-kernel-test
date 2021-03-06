
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

#include <asm-generic/vmlinux.lds.h>
#include <linux/memblock.h>

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

    //p = &__setup_early_memblock;
    //for (p = __setup_start; p < __setup_end; p++) {
        if ((p->early && parameq(param, p->str)) ||
            (strcmp(param, "console") == 0 &&
             strcmp(p->str, "earlycon") == 0)
        ) {
            if (p->setup_func(val) != 0)
                pr_warn("Malformed early option '%s'\n", param);
        }
    //}
#endif

    memblock_debug = 1;

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
    pr_fn_start_on(stack_depth);

    static int done __initdata;
    static char tmp_cmdline[COMMAND_LINE_SIZE] __initdata;

    if (done)
        return;

    /* All fall through to do_early_param. */
    strlcpy(tmp_cmdline, boot_command_line, COMMAND_LINE_SIZE);
    parse_early_options(tmp_cmdline);
    done = 1;

    pr_fn_end_on(stack_depth);
}
//493 lines


//511 lines
bool initcall_debug;



//574 lines
asmlinkage __visible void __init start_kernel(void)
{
    pr_fn_start_on(stack_depth);

    //test/init/start_kernel.c

    pr_fn_end_on(stack_depth);
}
