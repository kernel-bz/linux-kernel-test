// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/sched/sched-test.c
 *  Linux Kernel Schedular Test Module
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stdio.h>
#include <stdlib.h>

#include "test/test.h"
#include "test/debug.h"
#include <linux/sched/init.h>

int sched_test(void)
{
    pr_fn_start();
    sched_init();
    pr_fn_end();

    return 0;
}
