// SPDX-License-Identifier: GPL-2.0

#include <linux/sched/debug.h>
#include <linux/rwsem.h>
#include <linux/export.h>

void __sched down_read(struct rw_semaphore *sem)
{
    might_sleep();
    //rwsem_acquire_read(&sem->dep_map, 0, 0, _RET_IP_);

    //LOCK_CONTENDED(sem, __down_read_trylock, __down_read);
}
EXPORT_SYMBOL(down_read);

void __sched down_write(struct rw_semaphore *sem)
{
    might_sleep();
    //rwsem_acquire(&sem->dep_map, 0, 0, _RET_IP_);
    //LOCK_CONTENDED(sem, __down_write_trylock, __down_write);
}
EXPORT_SYMBOL(down_write);

void up_read(struct rw_semaphore *sem)
{
    //rwsem_release(&sem->dep_map, 1, _RET_IP_);
    //__up_read(sem);
}
EXPORT_SYMBOL(up_read);

void up_write(struct rw_semaphore *sem)
{
    //rwsem_release(&sem->dep_map, 1, _RET_IP_);
    //__up_write(sem);
}
EXPORT_SYMBOL(up_write);
