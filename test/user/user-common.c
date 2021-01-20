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
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/kernel.h>

#include <linux/notifier.h>
#include <linux/irqdomain.h>
#include <linux/phy.h>


//net/ethernet/eth.c: 573 lines
int nvmem_get_mac_address(struct device *dev, void *addrbuf)
{
    return 0;
}
EXPORT_SYMBOL(nvmem_get_mac_address);

