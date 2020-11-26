/* SPDX-License-Identifier: GPL-2.0 */
/*
 *	test/define-usr-dev.c
 *
 * 	Copyright (C) 2020, www.kernel.bz
 * 	Author: JaeJoon Jung <rgbi3307@gmail.com>
 *
 *	user defined functions for devices
 */
#ifndef __TEST_DEFINE_USR_LOCK_H
#define __TEST_DEFINE_USR_LOCK_H

#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/spinlock_types_up.h>
#include <linux/rwlock.h>
#include <linux/rwlock_types.h>


#define __lockfunc

//include/linux/rwlock_api_smp.h: 207 lines
static inline void __raw_write_lock(rwlock_t *lock)
{
    //preempt_disable();
    //rwlock_acquire(&lock->dep_map, 0, 0, _RET_IP_);
    //LOCK_CONTENDED(lock, do_raw_write_trylock, do_raw_write_lock);
}


//include/linux/rwlock_api_smp.h: 216 lines
static inline void __raw_write_unlock(rwlock_t *lock)
{
    //rwlock_release(&lock->dep_map, 1, _RET_IP_);
    //do_raw_write_unlock(lock);
    //preempt_enable();
}



#endif	//__TEST_DEFINE_USR_LOCK_H
