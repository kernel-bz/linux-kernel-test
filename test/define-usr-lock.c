/* SPDX-License-Identifier: GPL-2.0 */
/*
 *	test/define-usr-lock.c
 *
 * 	Copyright (C) 2020, www.kernel.bz
 * 	Author: JaeJoon Jung <rgbi3307@gmail.com>
 *
 *	user defined functions for lock
 */
#include "test/define-usr-lock.h"
#include <linux/rwlock_types.h>
#include <linux/export.h>


//kernel/locking/spinlock.c: 292 lines
#ifndef CONFIG_INLINE_WRITE_LOCK
void __lockfunc _raw_write_lock(rwlock_t *lock)
{
    __raw_write_lock(lock);
}
EXPORT_SYMBOL(_raw_write_lock);
#endif


//kernel/locking/spinlock.c: 324 lines
#ifndef CONFIG_INLINE_WRITE_UNLOCK
void __lockfunc _raw_write_unlock(rwlock_t *lock)
{
    __raw_write_unlock(lock);
}
EXPORT_SYMBOL(_raw_write_unlock);
#endif




//kernel/locking/mutex.c: 40 lines
void
__mutex_init(struct mutex *lock, const char *name, struct lock_class_key *key)
{
    //atomic_long_set(&lock->owner, 0);
    //spin_lock_init(&lock->wait_lock);
    //INIT_LIST_HEAD(&lock->wait_list);
#ifdef CONFIG_MUTEX_SPIN_ON_OWNER
    osq_lock_init(&lock->osq);
#endif

    //debug_mutex_init(lock, name, key);
}
EXPORT_SYMBOL(__mutex_init);

