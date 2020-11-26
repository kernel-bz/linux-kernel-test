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



#endif 	//__TEST_DEFINE_USR_DEV_H
