// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/dtb_test.h
 *  DTB Test Module
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

#include "test/user-types.h"

//0xd00dfeed
u8 dtb_test_blob[] = {
    0xd0, 0x0d, 0xfe, 0xed,
    0x00, 0x00, 0x00, 0x08,
};

void *dtb_early_va = (void*)dtb_test_blob;
