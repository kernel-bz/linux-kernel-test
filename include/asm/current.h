/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_X86_CURRENT_H
#define _ASM_X86_CURRENT_H

#include <linux/compiler.h>
//#include <asm/percpu.h>
#include <linux/percpu.h>

#ifndef __ASSEMBLY__
struct task_struct;

static DECLARE_PER_CPU(struct task_struct *, current_task);

static __always_inline struct task_struct *get_current(void)
{
    //return this_cpu_read_stable(current_task);
    return current_task;
}

#define current get_current()

#endif /* __ASSEMBLY__ */

#endif /* _ASM_X86_CURRENT_H */
