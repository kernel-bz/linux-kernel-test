// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/algorithm/algorithm.c
 *  Algorithm & Struct Trainging
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

#include <stdio.h>
#include <stdlib.h>

#include "test/debug.h"
#include "test/test.h"

static void _struct_algo_help(void)
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

static int _struct_algo_menu(int asize)
{
    int idx;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Struct & Algorithm Menu\n");
    printf(" 0: help.\n");
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
    printf("11: exit.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    scanf("%d", &idx);
    return (idx >= 0 && idx < asize) ? idx : -1;
}

void struct_algorithm(void)
{
    void (*fn[])(void) = { _struct_algo_help
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
    };
    int idx;
    int asize = sizeof (fn) / sizeof (fn[0]);

_retry:
    idx = _struct_algo_menu(asize);
    if (idx >= 0) {
        fn[idx]();
        goto _retry;
    }
}
