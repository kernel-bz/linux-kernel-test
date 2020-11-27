/* SPDX-License-Identifier: GPL-2.0 */
/*
 *	include/test/define-usr-dev.h
 *
 * 	Copyright (C) 2020, www.kernel.bz
 * 	Author: JaeJoon Jung <rgbi3307@gmail.com>
 *
 *	user defined header for devices
 */
#ifndef __TEST_DEFINE_USR_DEV_H
#define __TEST_DEFINE_USR_DEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "test/debug.h"

#include <stdbool.h>
#include <linux/types-user.h>
#include <linux/device.h>

//include/linux/dma-mapping.h: 700 lines
#ifdef CONFIG_ARCH_HAS_SETUP_DMA_OPS
void arch_setup_dma_ops(struct device *dev, u64 dma_base, u64 size,
        const struct iommu_ops *iommu, bool coherent);
#else
static inline void arch_setup_dma_ops(struct device *dev, u64 dma_base,
        u64 size, const struct iommu_ops *iommu, bool coherent)
{
}
#endif /* CONFIG_ARCH_HAS_SETUP_DMA_OPS */


//include/linux/irq.h: 268 lines
static inline u32 irqd_get_trigger_type(struct irq_data *d)
{
    return 0;
}


//include/linux/etherdevice.h: 191 lines
static inline bool is_valid_ether_addr(const u8 *addr)
{
    /* FF:FF:FF:FF:FF:FF is a multicast address so we don't need to
     * explicitly check for it here. */
    //return !is_multicast_ether_addr(addr) && !is_zero_ether_addr(addr);
    return true;
}


//drivers/base/power/power.h: 146 lines
static inline void device_pm_init(struct device *dev)
{
    //device_pm_init_common(dev);
    //device_pm_sleep_init(dev);
    //pm_runtime_init(dev);
}


//include/asm-generic/unaligned.h: 30 lines
//#define get_unaligned	__get_unaligned_be
#define get_unaligned(ptr)	0


#ifdef __cplusplus
}
#endif

#endif 	//__TEST_DEFINE_USR_DEV_H
