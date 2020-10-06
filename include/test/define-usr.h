#ifndef __TEST_DEFINE_USR_H
#define __TEST_DEFINE_USR_H

#include <linux/bitmap.h>
#include <linux/numa.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef sigset_t
typedef struct {
    unsigned long sig[2];
} sigset_t;
#endif


//include/linux/compiler.h
#define notrace
//#define __randomize_layout
//#define ____cacheline_aligned
//#define __cpuidle
//#define __init
//#define __maybe_unused
//#define __must_check
//#define __always_inline		inline
#define __initdata

#define lockdep_assert_held(l)			do { (void)(l); } while (0)

//include/linux/nodemask.h
typedef struct { DECLARE_BITMAP(bits, MAX_NUMNODES); } nodemask_t;

//include/linux/cgroup-defs.h
struct cgroup_subsys_state {
    int id;
    struct cgroup_subsys_state	*parent;
};

#define this_cpu_read_stable(var)

//include/linux/sched/signal.h
static inline int signal_pending_state(long state, struct task_struct *p) { }

//include/linux/raid/pq.h
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


#ifdef __cplusplus
}
#endif

#endif // __TEST_DEFINE_USR_H
