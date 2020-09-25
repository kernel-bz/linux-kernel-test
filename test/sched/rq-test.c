#include <stdio.h>
#include <stdlib.h>
#include "test/test.h"

/*
//kernel/sched/sched.h
DECLARE_PER_CPU_SHARED_ALIGNED(struct rq, runqueues);

#define cpu_rq(cpu)             (&per_cpu(runqueues, (cpu)))
#define this_rq()               this_cpu_ptr(&runqueues)
#define task_rq(p)              cpu_rq(task_cpu(p))
#define cpu_curr(cpu)           (cpu_rq(cpu)->curr)
#define raw_rq()                raw_cpu_ptr(&runqueues)

Studing in load_balance()...
*/

int rq_test(void)
{
    return 0;
}
