// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2009 Sunplus Core Technology Co., Ltd.
 *  Chen Liqin <liqin.chen@sunplusct.com>
 *  Lennox Wu <lennox.wu@sunplusct.com>
 * Copyright (C) 2012 Regents of the University of California
 */
#if defined(__aarch64__)
#include <string.h>

#include "test/debug.h"
#include "test/dtb-test.h"

#include <linux/init.h>
#include <linux/mm.h>
#include <linux/memblock.h>
#include <linux/sched.h>
//#include <linux/console.h>
#include <linux/screen_info.h>
#include <linux/of_fdt.h>
#include <linux/of_platform.h>
#include <linux/sched/task.h>
//#include <linux/swiotlb.h>

#include <asm/setup.h>
#include <asm/sections.h>
//#include <asm/pgtable.h>
//#include <asm/smp.h>
//#include <asm/tlbflush.h>
#include <asm/thread_info.h>

#include <asm-generic/vmlinux.lds.h>

//#include "head.h"

/* The lucky hart to first increment this variable will boot the other cores */
atomic_t hart_lottery;
unsigned long boot_cpu_hartid;

void __init parse_dtb(void)
{
    pr_fn_start_on(stack_depth);

    if (!strlen(dtb_file_name))
        strlcpy(dtb_file_name, CONFIG_USER_DTB_FILE, sizeof(dtb_file_name));
    dtb_test_read_file();

    if (early_init_dt_scan(dtb_early_va))
        return;

    pr_err("No DTB passed to the kernel\n");
#ifdef CONFIG_CMDLINE_FORCE
	strlcpy(boot_command_line, CONFIG_CMDLINE, COMMAND_LINE_SIZE);
	pr_info("Forcing kernel command line to: %s\n", boot_command_line);
#endif

    pr_info_view_on(stack_depth, "%20s : %s\n", boot_command_line);
    pr_fn_end_on(stack_depth);
}

void __init setup_arch(char **cmdline_p)
{
    pr_fn_start_on(stack_depth);

    //arch/arm64/kernel/vmlinux.lds.S:
    //SECTIONS
    //INIT_SETUP(16)

    parse_dtb();

	*cmdline_p = boot_command_line;

	parse_early_param();

    //setup_bootmem();
    //paging_init();
	unflatten_device_tree();

#ifdef CONFIG_SWIOTLB
	swiotlb_init(1);
#endif

#ifdef CONFIG_SMP
    //setup_smp();
#endif

    //riscv_fill_hwcap();

    pr_fn_end_on(stack_depth);
}

#endif
