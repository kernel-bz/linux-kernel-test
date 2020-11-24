#ifndef __TEST_DEFINE_USR_H
#define __TEST_DEFINE_USR_H

#include <linux/bitmap.h>
#include <linux/numa.h>
#include <asm/thread_info.h>
#include <linux/signal.h>
#include <linux/export.h>

#ifdef __cplusplus
extern "C" {
#endif

//include/linux/types.h
/* bsd */
typedef unsigned char 	u_char;
typedef unsigned short 	u_short;
typedef unsigned int 	u_int;
typedef unsigned long 	u_long;

//include/linux/compiler.h
#define notrace
#define __always_inline		inline
#define __initdata
#define __ref
#define __cpuidle
#define __malloc
#define __iomem
#define __percpu
#define __visible
#define __ro_after_init
#define __initconst
#define __used
#define __rmem_of_table_sentinel

#define lockdep_assert_held(l)			do { (void)(l); } while (0)

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

//arch/arm/include/asm/processor.h
//#define task_pt_regs(p) \
//    ((struct pt_regs *)(THREAD_START_SP + task_stack_page(p)) - 1)


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

//include/linux/sysfs.h
struct attribute {
    const char		*name;
    unsigned short	mode;
};

struct bin_attribute {
    struct attribute	attr;
    size_t				size;
    void				*private;
};

//scripts/dtc/libfdt/libfdt.h
#define FDT_ERR_NOTFOUND	1

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

//scripts/dtc/libfdt/fdt.h
typedef __be32 fdt32_t;
struct fdt_header {
    fdt32_t magic;			 /* magic word FDT_MAGIC */
    fdt32_t totalsize;		 /* total size of DT block */
    fdt32_t off_dt_struct;		 /* offset to structure */
    fdt32_t off_dt_strings;		 /* offset to strings */
    fdt32_t off_mem_rsvmap;		 /* offset to memory reserve map */
    fdt32_t version;		 /* format version */
    fdt32_t last_comp_version;	 /* last compatible version */

    /* version 2 fields below */
    fdt32_t boot_cpuid_phys;	 /* Which physical CPU id we're
                        booting on */
    /* version 3 fields below */
    fdt32_t size_dt_strings;	 /* size of the strings block */

    /* version 17 fields below */
    fdt32_t size_dt_struct;		 /* size of the structure block */
};




#ifdef __cplusplus
}
#endif

#endif // __TEST_DEFINE_USR_H
