/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Generic task switch macro wrapper.
 *
 * It should be possible to use these on really simple architectures,
 * but it serves more as a starting point for new ports.
 *
 * Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 */
#ifndef __ASM_GENERIC_SWITCH_TO_H
#define __ASM_GENERIC_SWITCH_TO_H

#include <linux/thread_info.h>
#include <linux/sched.h>
#include <asm/current.h>

/*
 * Context switching is now performed out-of-line in switch_to.S
 */
//extern struct task_struct *__switch_to(struct task_struct *,
//				       struct task_struct *);
static struct task_struct *__switch_to(struct task_struct *from,
                                struct task_struct *to)
{
    //to->thread.prev_sched = from;
    to->thread_info.task = from;
    //set_current(to);
    current_task = to;

    //switch_threads(&from->thread.switch_buf, &to->thread.switch_buf);

    return current_task;
}

#define switch_to(prev, next, last)					\
    do {											\
        ((last) = __switch_to((prev), (next)));		\
	} while (0)

#endif /* __ASM_GENERIC_SWITCH_TO_H */
