// SPDX-License-Identifier: GPL-2.0-only
/* Kernel thread helper functions.
 *   Copyright (C) 2004 IBM Corporation, Rusty Russell.
 *   Copyright (C) 2009 Red Hat, Inc.
 *
 * Creation is done via kthreadd, so that we get a clean environment
 * even if we're invoked from userspace (think modprobe, hotplug cpu,
 * etc.).
 */

#include <linux/sched.h>

struct task_struct *kthread_create_on_node(int (*threadfn)(void *data),
                                           void *data, int node,
                                           const char namefmt[],
                                           ...)
{
    struct task_struct *task;
    va_list args;

    va_start(args, namefmt);
    //task = __kthread_create_on_node(threadfn, data, node, namefmt, args);
    task = NULL;
    va_end(args);

    return task;
}
EXPORT_SYMBOL(kthread_create_on_node);
