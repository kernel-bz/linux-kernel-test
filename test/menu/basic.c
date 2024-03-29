// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/menu/basic.c
 *  Basic Trainging Menu
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>

#include "test/config.h"
#include "test/basic.h"
#include "test/test.h"
#include <linux/kernel.h>

static void _basic_training_help(void)
{
    //help messages...
    printf("\n");
    printf("  This is the Linux kernel source testing program\n");
    printf("that can be run at the user level.\n");
    printf("This menu is to train basic kernel source.\n");
    printf("written by www.kernel.bz\n");
    printf("Tested Kernel Version: v%d.%d-td%d\n",
           CONFIG_VERSION_1, CONFIG_VERSION_2, CONFIG_VERSION_3);
    printf("\n");
    return;
}

static int _basic_training_menu(int asize)
{
    int idx, ret;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Basic Training Menu\n");
    printf("0: exit.\n");
    printf("1: Data Types.\n");
    printf("2: Basic Pointer Test.\n");
    printf("3: Basic Pointer2 Test.\n");
    printf("4: Basic Struct Test.\n");
    printf("5: Basic Struct Test2.\n");
    printf("6: Basic Macro Test.\n");
    printf("7: Bits Operation Test.\n");
    printf("8: CPU Mask Test.\n");
    printf("9: Run Time(CPU cycles) Test.\n");
    printf("10: Sort Test.\n");
    printf("11: Hash Value Test.\n");
    printf("12: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize - 1);
    ret = scanf("%d", &idx);
    if (ret <= 0) idx = 0;

    return (idx > 0 && idx < asize) ? idx : -1;
}

void menu_basic_train(void)
{
    void (*fn[])(void) = { _basic_training_help
        , basic_types_test
        , basic_ptr_test
        , basic_ptr2_test
        , basic_struct_test
        , basic_struct_test2
        , basic_macro_test
        , bitmap_test
        , cpus_mask_test
        , basic_run_time_test
        , lib_sort_test
        , basic_lib_hashval_test
        , _basic_training_help
    };
    int idx;

    while(1) {
        idx = _basic_training_menu(ARRAY_SIZE(fn));
        if (idx < 0) break;
        fn[idx]();
    }
}
