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
    printf("Tis is the Linux kernel source testing program\n");
    printf("that can be run at the user level.\n");
    printf("written by www.kernel.bz\n");
    printf("Kernel Version: v%d.%d-td%d\n",
           CONFIG_VERSION_1, CONFIG_VERSION_2, CONFIG_VERSION_3);
    printf("\n");
    return;
}

static int _drivers_menu(int asize)
{
    int idx;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Drivers Test Menu\n");
    printf("0: exit.\n");
    printf("1: DTB Set FileName.\n");
    printf("2: DTB Read From File.\n");
    printf("3: DTB Hex Dump.\n");
    printf("4: Device Tree Test -->\n");
    printf("5: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    scanf("%d", &idx);
    return (idx > 0 && idx < asize) ? idx : -1;
}

void menu_drivers(void)
{
    void (*fn[])(void) = { _drivers_menu_help
        , dtb_set_file_name
        , dtb_test_read_file
        , dtb_test_hex_dump
        , dtb_of_unittest
        , _drivers_menu_help
    };
    int idx;
    int asize = sizeof (fn) / sizeof (fn[0]);

    while(1) {
        idx = _drivers_menu(asize);
        if (idx < 0) break;
        fn[idx]();
    }
}
