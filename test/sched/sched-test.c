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
#include <linux/sched/init.h>

int sched_test(void)
{
    printf("Starting sched_test()...\n");
    decay_load_test();
    update_load_avg_test();

    printf("Starting sched_init()...\n");
    sched_init();
    return 0;
}
