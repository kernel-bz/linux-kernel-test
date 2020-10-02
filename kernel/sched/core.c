// SPDX-License-Identifier: GPL-2.0-only
/*
 *  kernel/sched/core.c
 *
 *  Core kernel scheduler code and related syscalls
 *
 *  Copyright (C) 1991-2002  Linus Torvalds
 */
#include "sched.h"

//#include <linux/nospec.h>

#include <linux/kcov.h>

//#include <asm/switch_to.h>
//#include <asm/tlb.h>

//#include "../workqueue_internal.h"
//#include "../smpboot.h"

#include "pelt.h"

#define CREATE_TRACE_POINTS
//#include <trace/events/sched.h>


//DEFINE_PER_CPU_SHARED_ALIGNED(struct rq, runqueues);
DEFINE_PER_CPU(struct rq, runqueues);

