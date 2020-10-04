#ifndef __LINUX_LOCKDEP_H
#define __LINUX_LOCKDEP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/spinlock.h>

#define MAX_LOCK_DEPTH	63UL

#define asmlinkage
#define __visible


struct lock_class_key {
	unsigned int a;
};

static inline void lockdep_set_class(spinlock_t *lock,
					struct lock_class_key *key)
{
}

#define SINGLE_DEPTH_NESTING			1

extern struct task_struct *__curr(void);

#define current (__curr())

static inline int debug_locks_off(void)
{
    return 1;
}

#define KSYM_NAME_LEN 128

#define list_del_rcu list_del

#define static_obj(x) 1

#define debug_show_all_locks()
extern void debug_check_no_locks_held(void);

static __used bool __is_kernel_percpu_address(unsigned long addr, void *can_addr)
{
    return false;
}


#ifdef __cplusplus
}
#endif

#endif /* _LINUX_LOCKDEP_H */
