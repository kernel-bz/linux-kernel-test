#ifndef __TEST_DEFINE_USR_H
#define __TEST_DEFINE_USR_H

#include <linux/bitmap.h>
#include <linux/numa.h>
#include <asm/thread_info.h>
//#include <linux/sched/task_stack.h>
#include <linux/signal.h>


#ifdef __cplusplus
extern "C" {
#endif

//include/linux/compiler.h
#define notrace
//#define __randomize_layout
//#define ____cacheline_aligned
//#define __cpuidle
//#define __init
//#define __maybe_unused
//#define __must_check
#define __always_inline		inline
#define __initdata
#define __ref
#define __cpuidle
#define __malloc
#define __iomem
#define __percpu

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
#define IS_ENABLED(x) (x)

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

//include/linux/pm.h
typedef struct pm_message {
    int event;
} pm_message_t;

struct dev_pm_info {

};


#ifdef __cplusplus
}
#endif

#endif // __TEST_DEFINE_USR_H
