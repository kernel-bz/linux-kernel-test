// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2009 Sunplus Core Technology Co., Ltd.
 *  Chen Liqin <liqin.chen@sunplusct.com>
 *  Lennox Wu <lennox.wu@sunplusct.com>
 * Copyright (C) 2012 Regents of the University of California
 */
#include "test/config.h"
#if CONFIG_RUN_ARCH == ARCH_X86 || CONFIG_RUN_ARCH == ARCH_X86_64

#include "test/debug.h"

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

//#include "head.h"


/* The lucky hart to first increment this variable will boot the other cores */
atomic_t hart_lottery;
unsigned long boot_cpu_hartid;

void __init parse_dtb(void)
{
    //if (early_init_dt_scan(dtb_early_va))
        //return;

    //pr_err("No DTB passed to the kernel\n");
#ifdef CONFIG_CMDLINE_FORCE
	strlcpy(boot_command_line, CONFIG_CMDLINE, COMMAND_LINE_SIZE);
	pr_info("Forcing kernel command line to: %s\n", boot_command_line);
#endif
}

void __init setup_arch(char **cmdline_p)
{
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
}

#endif
