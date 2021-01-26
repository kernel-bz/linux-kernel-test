// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/dtb_test.h
 *  DTB Test Module
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#ifndef __DTB_TEST_H
#define __DTB_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "test/user-types.h"

//test/config/config-set.c
extern char dtb_file_name[80];
extern void *dtb_early_va;
extern int dtb_size;

void dtb_set_file_name(void);
void dtb_test_read_file(void);
void dtb_test_hex_dump(void);

#ifdef __cplusplus
}
#endif

#endif
