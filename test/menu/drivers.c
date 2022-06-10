// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/menu/config.c
 *  Drivers Test Menu
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>

#include "test/debug.h"
#include "test/config.h"
#include "test/test.h"
#include "test/dtb-test.h"

static void _drivers_menu_help(void)
{
    //help messages...
    printf("\n");
    printf("  This menu is to test Linux kernel device driver sources\n");
    printf("at the user level as follows:\n");
    printf("written by www.kernel.bz\n");
    printf("Tested Kernel Version: v%d.%d-td%d\n",
           CONFIG_VERSION_1, CONFIG_VERSION_2, CONFIG_VERSION_3);
    printf("\n");
    return;
}

static int _dtb_unittest_menu(int asize)
{
    int idx, ret;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Device Tree --> UnitTest Menu\n");
    printf("  0: exit.\n");
    printf("  1: unittest_data_add\n");
    printf("  2: check_tree_linkage\n");
    printf("  3: find_node_by_name\n");
    printf("  4: dynamic\n");
    printf(" *5: check_phandles\n");
    printf(" *6: parse_phandle_with_args\n");
    printf(" *7: parse_phandle_with_args_map\n");
    printf(" *8: printf\n");
    printf(" *9: property_string\n");
    printf("*10: property_copy\n");
    printf("*11: changeset\n");
    printf("*12: parse_interrupts\n");
    printf("*13: parse_interrupts_extended\n");
    printf(" 14: match_node\n");
    printf("*15: platform_populate\n");

    printf(" 16: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize - 1);
    ret = scanf("%d", &idx);
    if (ret <= 0) idx = 0;

    return (idx > 0 && idx < asize) ? idx : -1;
}

static void _menu_of_unittest(void)
{
    void (*fn[])(void) = { _drivers_menu_help
        , dtb_unittest_data_add
        , dtb_unittest_check_tree_linkage
        , dtb_unittest_find_node_by_name
        , dtb_unittest_dynamic
        , dtb_unittest_check_phandles
        , dtb_unittest_parse_phandle_with_args
        , dtb_unittest_parse_phandle_with_args_map
        , dtb_unittest_printf
        , dtb_unittest_property_string
        , dtb_unittest_property_copy
        , dtb_unittest_changeset
        , dtb_unittest_parse_interrupts
        , dtb_unittest_parse_interrupts_extended
        , dtb_unittest_match_node
        , dtb_unittest_platform_populate
        , _drivers_menu_help
    };
    int idx;

    while (1) {
        idx = _dtb_unittest_menu(ARRAY_SIZE(fn));
        if (idx < 0) break;
        fn[idx]();
    }
}

static int _drivers_menu(int asize)
{
    int idx, ret;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Drivers Test Menu\n");
    printf("0: exit.\n");
    printf("1: DTB Set FileName.\n");
    printf("2: DTB Read From File.\n");
    printf("3: DTB Hex Dump.\n");
    printf("4: DTB print all nodes.\n");
    printf("5: Device Tree Test -->\n");
    printf("6: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize - 1);
    ret = scanf("%d", &idx);
    if (ret <= 0) idx = 0;

    return (idx > 0 && idx < asize) ? idx : -1;
}

void menu_drivers(void)
{
    void (*fn[])(void) = { _drivers_menu_help
        , dtb_set_file_name
        , dtb_test_read_file
        , dtb_test_hex_dump
        , dtb_test_print_allnodes
        , _menu_of_unittest
        , _drivers_menu_help
    };
    int idx;

    while(1) {
        idx = _drivers_menu(ARRAY_SIZE(fn));
        if (idx < 0) break;
        fn[idx]();
    }
}
