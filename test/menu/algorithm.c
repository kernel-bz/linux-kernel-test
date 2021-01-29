// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/menu/algorithm.c
 *  Algorithm & Struct Trainging Menu
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

#include <stdio.h>
#include <stdlib.h>

#include "test/debug.h"
#include "test/test.h"

static void _algo_struct_help(void)
{
    //help messages...
    printf("\n");
    printf("  This is the Linux kernel source testing program\n");
    printf("that can be run at the user level.\n");
    printf("This menu is to study kernel algorithms.\n");
    printf("written by www.kernel.bz\n");
    printf("Tested Kernel Version: v%d.%d-td%d\n",
           CONFIG_VERSION_1, CONFIG_VERSION_2, CONFIG_VERSION_3);
    printf("\n");
    return;
}

static int _algo_struct_menu(int asize)
{
    int idx, ret;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Algorithm & Struct Menu\n");
    printf(" 0: exit.\n");
    printf(" 1: Linked List Test01.\n");
    printf(" 2: Linked List Test02.\n");
    printf(" 3: Linked List Test03.\n");
    printf(" 4: Linked List Test04.\n");
    printf(" 5: List Sort Test.\n");
    printf(" 6: Red-Black Tree Test01.\n");
    printf(" 7: Red-Black Tree Test02.\n");
    printf(" 8: Red-Black Tree Test03.\n");
    printf(" 9: IDA Test.\n");
    printf("10: Xarray Test.\n");
    printf("11: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    ret = scanf("%d", &idx);
    if (ret <= 0) idx = 0;

    return (idx > 0 && idx < asize) ? idx : -1;
}

void menu_algorithm(void)
{
    void (*fn[])(void) = { _algo_struct_help
        , list_test01
        , list_test02
        , list_test03
        , list_test04
        , lib_list_sort_test
        , rbtree_test01
        , rbtree_test02
        , rbtree_test03
        , lib_ida_test
        , lib_xarray_test
        , _algo_struct_help
    };
    int idx;
    int asize = sizeof (fn) / sizeof (fn[0]);

    while(1) {
        idx = _algo_struct_menu(asize);
        if (idx < 0) break;
        fn[idx]();
    }
}
