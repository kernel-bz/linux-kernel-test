// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/drivers/dtb_test.c
 *  DTB Test Module
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

#include "test/config.h"
#include "test/user-types.h"
#include "test/dtb-test.h"
#include "test/debug.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <linux/kern_levels.h>
#include <linux/of.h>

char dtb_file_name[80];

//hexdump *.dtb
//0xd00dfeed
//0dd0 edfe 0000 a717 0000 3800 0000 3015
//2Bytes Big-Edian
u8 dtb_test_blob[] = {
    0xd0,0x0d,0xfe,0xed, 0x00,0x00,0x17,0xa7,
};

void *dtb_early_va = (void*)dtb_test_blob;
int dtb_size;

void dtb_set_file_name(void)
{
    char temp[80];

    if (!strlen(dtb_file_name))
        strlcpy(dtb_file_name, CONFIG_USER_DTB_FILE, sizeof(dtb_file_name));

    strcpy(temp, dtb_file_name);

    __fpurge(stdin);
    printf("Enter DTB File Name[%s]: ", dtb_file_name);
    scanf("%s", dtb_file_name);

    if (strlen(dtb_file_name) < 2)
        strcpy(dtb_file_name, temp);
}

void dtb_test_read_file(void)
{
    int fd, ret, size=0, size2;
    u8 buf[1024];
    u8 *blob=NULL, *blob2;

    pr_fn_start(stack_depth);

    if (!strlen(dtb_file_name))
        strlcpy(dtb_file_name, CONFIG_USER_DTB_FILE, sizeof(dtb_file_name));

    fd = open(dtb_file_name, O_RDONLY);
    if (fd < 0) {
        pr_err("Open File(%s) Error!", dtb_file_name);
        return -1;
    }

    if ((void*)dtb_early_va != (void*)dtb_test_blob)
        if (dtb_size > 0 && dtb_early_va)
            free(dtb_early_va);        //free old allocation

    pr_view_on(stack_depth, "%20s : %s\n", dtb_file_name);

    while (1) {
        ret = read(fd, buf, sizeof(buf));
        if (ret <= 0) break;
        //pr_view_on(stack_depth, "%20s : %d bytes\n", ret);
        size += ret;
        if (size > ret) {
            size2 = size - ret;
            blob2 = realloc(blob, size);
        } else {
            size2 = 0;
            blob2 = malloc(size);
        }
        if (!blob2) {
            pr_err("memory alloc error!\n");
            if (blob) free(blob);
            goto _exit;
        }
        memcpy(blob2+size2, buf, ret);
        blob = blob2;
    }

    dtb_early_va = (void*)blob;

_exit:
    close(fd);
    dtb_size = size;

    pr_view_on(stack_depth, "%20s : %d bytes\n", dtb_size);

    pr_fn_end(stack_depth);
}

#if 0
void dtb_test_hex_dump(void)
{
    pr_fn_start_on(stack_depth);

    int i, ret;
    char *buf;

    buf = malloc(dtb_size*10);
    //lib/hex_dump.c
    ret = hex_dump_to_buffer(dtb_early_va, dtb_size, 16, 1
            , buf, dtb_size*10, true);

    pr_view_on(stack_depth, "%20s : %d\n", ret);
    pr_view_on(stack_depth, "%20s : %d\n", dtb_size);

    for (i=0; i<dtb_size; i++)
        printf("%c", buf[i]);

    pr_fn_end_on(stack_depth);
}
#endif

void dtb_test_hex_dump(void)
{
    pr_fn_start_on(stack_depth);

    //lib/hex_dump.c
    print_hex_dump(KERN_DEBUG, "raw data: ", DUMP_PREFIX_OFFSET,
            16, 1, dtb_early_va, dtb_size, true);

    pr_view_on(stack_depth, "%20s : %s\n", dtb_file_name);
    pr_view_on(stack_depth, "%20s : %d\n", dtb_size);
    pr_fn_end_on(stack_depth);
}

void dtb_test_print_allnodes(void)
{
    struct device_node *np;

    of_fdt_unflatten_tree(dtb_early_va, NULL, &np);
    pr_view_on(stack_depth, "%20s : %p\n", np);

    if (np) {
        of_root = np;
        for_each_of_allnodes(np)
                pr_view_on(stack_depth, "%20s : %s\n", np->name);
    } else {
        pr_warn("%s: No device tree to attach\n", __func__);
    }
}
