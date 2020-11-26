/* SPDX-License-Identifier: GPL-2.0 */
/*
 *	include/test/define-usr.h
 *
 * 	Copyright (C) 2020, www.kernel.bz
 * 	Author: JaeJoon Jung <rgbi3307@gmail.com>
 *
 *	user defined header
 */
#ifndef __TEST_DEFINE_USR_H
#define __TEST_DEFINE_USR_H

#include <stdarg.h>
#include <stddef.h>
#include <assert.h>
#include <endian.h>
#include <byteswap.h>

#include <linux/compiler.h>
#include <linux/compiler_attributes.h>
#include <linux/bitmap.h>
#include <linux/numa.h>
#include <linux/signal.h>
#include <linux/export.h>
#include <linux/kobject.h>
#include <linux/kernel.h>

#include <asm/thread_info.h>

//include/linux/types.h
/* bsd */
typedef unsigned char 	u_char;
typedef unsigned short 	u_short;
typedef unsigned int 	u_int;
typedef unsigned long 	u_long;

//include/linux/compiler.h
#define notrace
#define __initdata
#define __ref
#define __cpuidle
#define __malloc
#define __iomem
#define __percpu
#define __visible
#define __ro_after_init
#define __initconst
#define __rmem_of_table_sentinel
#define __randomize_layout
#define __user
#define __refdata
#define __initdata_memblock

#define lockdep_assert_held(l)	do { (void)(l); } while (0)

//include/linux/byteorder/generic.h
//tools/include/linux/kernel.h
//usr/include/byteswap.h
#if (__BYTE_ORDER == __BIG_ENDIAN)
#define cpu_to_le16 bswap_16
#define cpu_to_le32 bswap_32
#define cpu_to_le64 bswap_64
#define le16_to_cpu bswap_16
#define le32_to_cpu bswap_32
#define le64_to_cpu bswap_64
#define cpu_to_be16
#define cpu_to_be32
#define cpu_to_be64
#define be16_to_cpu
#define be32_to_cpu
#define be64_to_cpu
#else
#define cpu_to_le16
#define cpu_to_le32
#define cpu_to_le64
#define le16_to_cpu
#define le32_to_cpu
#define le64_to_cpu
#define cpu_to_be16 bswap_16
#define cpu_to_be32 bswap_32
#define cpu_to_be64 bswap_64
#define be16_to_cpu bswap_16
#define be32_to_cpu bswap_32
#define be64_to_cpu bswap_64

#define be16_to_cpup bswap_16
#define be32_to_cpup bswap_32
#define be64_to_cpup bswap_64
#endif

//include/linux/byteorder/generic.h
//#define be32_to_cpup __be32_to_cpup
//#define be32_to_cpus __be32_to_cpus


//include/linux/byteorder/generic.h
static inline void be32_add_cpu(__be32 *var, u32 val)
{
    *var = cpu_to_be32(be32_to_cpu(*var) + val);
}


//include/linux/atomic-fallback.h
#ifndef atomic_inc_return
static inline int
atomic_inc_return(atomic_t *v)
{
    //return atomic_add_return(1, v);
    v->counter += 1;
    return v->counter;

}
#define atomic_inc_return atomic_inc_return
#endif


#define this_cpu_read_stable(var)

//include/linux/sched/signal.h
static inline int signal_pending_state(long state, struct task_struct *p) { }

//include/linux/kconfig.h
//#define IS_ENABLED(x) (x)

//include/linux/kmemleak.h
static inline void kmemleak_update_trace(const void *ptr) { }

//include/asm-generic/topology.h
#ifndef cpumask_of_node
  #ifdef CONFIG_NEED_MULTIPLE_NODES
    #define cpumask_of_node(node)	((node) == 0 ? cpu_online_mask : cpu_none_mask)
  #else
    #define cpumask_of_node(node)	((void)(node), cpu_online_mask)
  #endif
#endif

//include/linux/sched/cputime.h
static inline void account_group_exec_runtime(struct task_struct *tsk,
                          unsigned long long ns)
{ }

//tools/testing/radix-tree/idr-test.c
#define dump_stack()    assert(0)

/* Is there a portable way to do this? */
#if defined(__x86_64__) || defined(__i386__)
#define cpu_relax() asm ("rep; nop" ::: "memory")
#elif defined(__s390x__)
#define cpu_relax() barrier()
#else
#define cpu_relax() assert(0)
#endif

//include/linux/sched/signal.h
#define for_each_process_thread(p, t)

//kernel/locking/spinlock.c
#define _raw_read_lock(lock)
#define _raw_read_unlock(lock)

//kernel/printk/printk.c
static int preferred_console = -1;
int console_set_on_cmdline;
EXPORT_SYMBOL(console_set_on_cmdline);

#define FW_BUG		"[Firmware Bug]: "
#define FW_WARN		"[Firmware Warn]: "
#define FW_INFO		"[Firmware Info]: "

//include/linux/of_fdt.h
void *initial_boot_params;

//include/uapi/linux/if_ether.h
#define ETH_ALEN	6		/* Octets in one ethernet addr	 */
#define ETH_TLEN	2		/* Octets in ethernet type field */
#define ETH_HLEN	14		/* Total octets in header.	 */
#define ETH_ZLEN	60		/* Min. octets in frame sans FCS */
#define ETH_DATA_LEN	1500		/* Max. octets in payload	 */
#define ETH_FRAME_LEN	1514		/* Max. octets in frame sans FCS */
#define ETH_FCS_LEN	4		/* Octets in the FCS		 */

#define ETH_MIN_MTU	68		/* Min IPv4 MTU per RFC791	*/
#define ETH_MAX_MTU	0xFFFFU		/* 65535, same as IP_MAX_MTU	*/

//include/linux/mmzone.h
#ifndef CONFIG_FORCE_MAX_ZONEORDER
#define MAX_ORDER 11
#else
#define MAX_ORDER CONFIG_FORCE_MAX_ZONEORDER
#endif

#define ARM64_HW_PGTABLE_LEVEL_SHIFT(n)	((PAGE_SHIFT - 3) * (4 - (n)) + 3)
#define PMD_SHIFT			ARM64_HW_PGTABLE_LEVEL_SHIFT(2)
#define HUGE_MAX_HSTATE		4
#define HPAGE_SHIFT			PMD_SHIFT
#define HPAGE_SIZE			(_AC(1, UL) << HPAGE_SHIFT)
#define HPAGE_MASK			(~(HPAGE_SIZE - 1))
#define HUGETLB_PAGE_ORDER	(HPAGE_SHIFT - PAGE_SHIFT)

#define pageblock_order		HUGETLB_PAGE_ORDER

//include/linux/acpi.h
#define acpi_disabled 1

//include/linux/pci.h
static inline unsigned long pci_address_to_pio(phys_addr_t addr) { return -1; }

//include/linux/dma-mapping.h
#define DMA_BIT_MASK(n)	(((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))
#define DMA_MASK_NONE	0x0ULL

//include/linux/mm.h
#define PAGE_ALIGNED(addr)	IS_ALIGNED((unsigned long)(addr), PAGE_SIZE)


#endif // __TEST_DEFINE_USR_H
