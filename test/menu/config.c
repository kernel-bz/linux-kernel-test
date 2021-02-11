// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/menu/config.c
 *  Config Setting Menu
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

static void _config_setting_help(void)
{
    //help messages...
    printf("\n");
    printf("  This is the Linux kernel source testing program\n");
    printf("that can be run at the user level.\n");
    printf("This menu is to set the kernel configuration.\n");
    printf("written by www.kernel.bz\n");
    printf("Tested Kernel Version: v%d.%d-td%d\n",
           CONFIG_VERSION_1, CONFIG_VERSION_2, CONFIG_VERSION_3);
    printf("\n");
    return;
}

static int _config_setting_menu(int asize)
{
    int idx, ret;

    printf("\n");
    printf("[#]--> Config Setting Menu\n");
    printf("0: exit.\n");
    printf("1: Config View.\n");
    printf("2: Set Debug Level.\n");
    printf("3: Set DTB FileName.\n");
    printf("4: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);

    __fpurge(stdin);
    ret = scanf("%d", &idx);
    if (ret <= 0) idx = 0;

    return (idx > 0 && idx < asize) ? idx : -1;
}

void menu_config(void)
{
    void (*fn[])(void) = { _config_setting_help
        , config_view
        , config_set_debug
        , dtb_set_file_name
        , _config_setting_help
    };
    int idx;
    int asize = sizeof (fn) / sizeof (fn[0]);

    while(1) {
        idx = _config_setting_menu(asize);
        if (idx < 0) break;
        fn[idx]();
    }
}
