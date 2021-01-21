// SPDX-License-Identifier: GPL-2.0
/*
 *  test/user/user-common.c
 *  User Functions for common
 *
 *  Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stddef.h>

#include <linux/device.h>
#include <linux/export.h>
#include <linux/kernel.h>


//net/ethernet/eth.c: 573 lines
int nvmem_get_mac_address(struct device *dev, void *addrbuf)
{
    return 0;
}
EXPORT_SYMBOL(nvmem_get_mac_address);

