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

//drivers/of/unittest.c
void dtb_unittest_data_add(void);
void dtb_unittest_check_tree_linkage(void);
void dtb_unittest_check_phandles(void);
void dtb_unittest_find_node_by_name(void);
void dtb_unittest_dynamic(void);
void dtb_unittest_parse_phandle_with_args(void);
void dtb_unittest_parse_phandle_with_args_map(void);
void dtb_unittest_printf(void);
void dtb_unittest_property_string(void);
void dtb_unittest_property_copy(void);
void dtb_unittest_changeset(void);
void dtb_unittest_parse_interrupts(void);
void dtb_unittest_parse_interrupts_extended(void);
void dtb_unittest_match_node(void);
void dtb_unittest_platform_populate(void);


#ifdef __cplusplus
}
#endif

#endif
