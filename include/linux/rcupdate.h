/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _RCUPDATE_H
#define _RCUPDATE_H

#include <urcu.h>
#include <urcu-pointer.h>
#include <linux/preempt.h>

#if 0
static inline int rcu_read_lock_held(void)
{
    return 1;
}

static inline int rcu_read_lock_bh_held(void)
{
    return 1;
}

static inline int rcu_read_lock_sched_held(void)
{
    return !preemptible();
}

static inline int rcu_read_lock_any_held(void)
{
    return !preemptible();
}
#endif

#define rcu_dereference(p)		(p)

#define rcu_dereference_raw(p) 				rcu_dereference(p)
#define rcu_dereference_protected(p, cond) 	rcu_dereference(p)
#define rcu_dereference_check(p, cond) 		rcu_dereference(p)
#define RCU_INIT_POINTER(p, v)		do { (p) = (v); } while (0)

#define rcu_dereference_sched(p)  	rcu_dereference(p)
#define rcu_dereference_sym(p)  	rcu_dereference(p)

//urcu.h
//#define rcu_read_lock()  			{ }
//#define rcu_read_unlock()  		{ }
#define rcu_read_lock_memb()		{ }
#define rcu_read_unlock_memb()		{ }

#define rcu_read_lock_sched()  		{ }
#define rcu_read_unlock_sched()  	{ }

#define rcu_lock_acquire(a)			do { } while (0)
#define rcu_lock_release(a)			do { } while (0)

#define RCU_LOCKDEP_WARN(c, s)		do { } while(0)

#define urcu_memb_read_lock(a)		do { } while (0)
#define urcu_memb_read_unlock(a)	do { } while (0)
#define urcu_memb_call_rcu(a, b)	{ }

#define rcu_set_pointer_sym(a, b)
#define rcu_assign_pointer(p, v)	do { (p) = (v); } while (0)

#endif
