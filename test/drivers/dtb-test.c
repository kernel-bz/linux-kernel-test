// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/drivers/dtb_test.c
 *  DTB Test Module
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

#include "test/user-types.h"
#include "test/dtb-test.h"
#include "test/debug.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

char dtb_file_name[80];

//hexdump *.dtb
//0xd00dfeed
//0dd0 edfe 0000 a717 0000 3800 0000 3015
//2Bytes Big-Edian
u8 dtb_test_blob[] = {
    0xd0,0x0d,0xfe,0xed, 0x00,0x00,0x17,0xa7,
    0x00, 0x00, 0x00, 0x08,
};

//void *dtb_early_va = (void*)dtb_test_blob;
void *dtb_early_va;
int dtb_size;

static void _dtb_set_file_name(void)
{
    __fpurge(stdin);
    printf("Enter DTB File Name: ");
    scanf("%s", dtb_file_name);
}

void dtb_test_read_file(void)
{
    pr_fn_start_on(stack_depth);

    int fd, ret, size=0, size2;
    u8 buf[80];
    u8 *blob, blob2;

    _dtb_set_file_name();

    fd = open(dtb_file_name, O_RDONLY);
    if (fd < 0) {
        pr_err("Open File(%s) Error!", dtb_file_name);
        return -1;
    }

    do {
        ret = read(fd, buf, sizeof(buf));
        if (ret <= 0) break;
        pr_info_view_on(stack_depth, "%20s : %d\n", ret);
        size += ret;
        if (size > ret) {
            size2 = size - ret;
            blob2 = realloc(blob, size);
            //memcpy(blob2, blob, size2);
            //memcpy(blob2+size2, buf, ret);
            //blob = blob2;
        } else {
            blob = malloc(size);
            memcpy(blob, buf, ret);
        }
    } while (blob);

    dtb_early_va = (void*)blob;

    close(fd);
    dtb_size = size;

    pr_info_view_on(stack_depth, "%20s : %d\n", dtb_size);

    pr_fn_end_on(stack_depth);
}

void dtb_test_hex_dump(void)
{
    pr_fn_start_on(stack_depth);

    int i, ret;
    char *buf;

    buf = malloc(dtb_size*10);
    //lib/hex_dump.c
    ret = hex_dump_to_buffer(dtb_early_va, dtb_size, 16, 1
            , buf, dtb_size*10, true);

    pr_info_view_on(stack_depth, "%20s : %d\n", ret);

    for (i=0; i<ret; i++)
        printf("%c", buf[i]);

    pr_fn_end_on(stack_depth);
}
