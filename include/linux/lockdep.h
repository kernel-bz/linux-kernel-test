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


#ifdef __cplusplus
}
#endif

#endif /* _LINUX_LOCKDEP_H */
