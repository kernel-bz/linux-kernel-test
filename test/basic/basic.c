// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/basic/basic.c
 *  Basic Trainging
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

#include <stdio.h>
#include <stdlib.h>

#include "test/config.h"
#include "test/basic.h"

static void _basic_training_help(void)
{
    //help messages...
    printf("\n");
    printf("Tis is the Linux kernel source testing program\n");
    printf("that can be run at the user level.\n");
    printf("written by www.kernel.bz\n");
    printf("Kernel Version: v%d.%d-td%d\n",
           CONFIG_VERSION_1, CONFIG_VERSION_2, CONFIG_VERSION_3);
    printf("\n");
    return;
}

static int _basic_training_menu(int asize)
{
    int idx;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Basic Training Menu\n");
    printf("0: help.\n");
    printf("1: Data Types.\n");
    printf("2: Bits Operation Test.\n");
    printf("3: CPU Mask Test.\n");
    printf("4: exit.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    scanf("%d", &idx);
    return (idx >= 0 && idx < asize) ? idx : -1;
}

void basic_training(void)
{
    void (*fn[])(void) = { _basic_training_help
        , basic_types_test
        , cpus_mask_test
        , cpus_mask_test
    };
    int idx;
    int asize = sizeof (fn) / sizeof (fn[0]);

_retry:
    idx = _basic_training_menu(asize);
    if (idx >= 0) {
        fn[idx]();
        goto _retry;
    }
}
