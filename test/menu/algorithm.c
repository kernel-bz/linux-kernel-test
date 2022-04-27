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
#include <linux/kernel.h>

static void _algo_struct_help(void)
{
    //help messages...
    printf("\n");
    printf("  This is the Linux kernel source testing program\n");
    printf("that can be run at the user space.\n");
    printf("This menu is to study kernel algorithms.\n");
    printf("written by www.kernel.bz\n");
    printf("Tested Kernel Version: v%d.%d-td%d\n",
           CONFIG_VERSION_1, CONFIG_VERSION_2, CONFIG_VERSION_3);
    printf("\n");
    return;
}

static int _algo_xarray_menu(int asize)
{
    int idx, ret;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Algorithm --> XArray Test Menu\n");
    printf(" 0: exit.\n");
    printf(" 1: XArray Info.\n");
    printf(" 2: XArray Simple Test.\n");
    printf(" 3: XArray Store Range Test.\n");
    printf(" 4: XArray Marks Test.\n");
    printf(" 5: XArray Check Test.\n");
    printf(" 6: XArray MultiOrder Test.\n");
    printf(" 7: IDR Simple Test.\n");
    printf(" 8: IDA Simple Test.\n");
    printf(" 9: IDR, IDA Check Test.\n");
    printf("10: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize - 1);
    ret = scanf("%d", &idx);
    if (ret <= 0) idx = 0;

    return (idx > 0 && idx < asize) ? idx : -1;
}

static void _algo_xarray_test(void)
{
    void (*fn[])(void) = { _algo_struct_help
        , xa_constants_view
        , xarray_test_simple
        , xarray_test_store_range
        , xarray_test_marks
        , xarray_test_run
        , xa_multiorder_test
        //, xa_main_test		//error
        , idr_simple_test
        , ida_simple_test
        //, idr_ida_main_test	//error
        , lib_ida_test
        , _algo_struct_help
    };
    int idx;

    while (1) {
        idx = _algo_xarray_menu(ARRAY_SIZE(fn));
        if (idx < 0) break;
        fn[idx]();
    }
}

static int _algo_maple_menu(int asize)
{
    int idx, ret;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Algorithm --> Maple Tree Test Menu\n");
    printf("0: exit.\n");
    printf("1: Maple Tree Info.\n");
    printf("2: Basic Store Test.\n");
    printf("3: Basic Store Range Test.\n");
    printf("4: Basic Walk Test.\n");
    printf("5: Store Loop Test.\n");
    printf("6: Store Load Erase Test.\n");
    printf("7: Maple Tree Main Test.\n");

    printf("8: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize - 1);
    ret = scanf("%d", &idx);
    if (ret <= 0) idx = 0;

    return (idx > 0 && idx < asize) ? idx : -1;
}

static void _algo_maple_tree_test(void)
{
    void (*fn[])(void) = { _algo_struct_help
        , mtree_info
        , mtree_basic_store_test
        , mtree_basic_store_range_test
        , mtree_basic_walk_test
        , mtree_store_loop_test
        , mtree_store_load_erase_test
        , maple_main_test

        , _algo_struct_help
    };
    int idx;

    while (1) {
        idx = _algo_maple_menu(ARRAY_SIZE(fn));
        if (idx < 0) break;
        fn[idx]();
    }
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
    printf(" 9: XArray Test -->\n");
    printf("10: Maple Tree Test -->\n");
    printf("11: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize - 1);
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
        , _algo_xarray_test
        , _algo_maple_tree_test
        , _algo_struct_help
    };
    int idx;

    while(1) {
        idx = _algo_struct_menu(ARRAY_SIZE(fn));
        if (idx < 0) break;
        fn[idx]();
    }
}
