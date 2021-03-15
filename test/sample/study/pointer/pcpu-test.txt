#include <stdio.h>

struct rq {
    char a;
    int b;
    int c;
    int d;
};

#define per_cpu_ptr(ptr, cpu)   ({ (void)(cpu); (ptr); })
#define per_cpu(var, cpu)	(*per_cpu_ptr(&(var), cpu))

#define DECLARE_PER_CPU_SHARED_ALIGNED(type, name)	type name

DECLARE_PER_CPU_SHARED_ALIGNED(struct rq, runqueues);

#define cpu_rq(cpu)		(&per_cpu(runqueues, (cpu)))
//#define task_rq(p)		cpu_rq(task_cpu(p))
#define cpu_curr(cpu)		(cpu_rq(cpu)->curr)
//#define raw_rq()		raw_cpu_ptr(&runqueues)

int main()
{
    int i;
    //struct rq *rq;

    printf("rq=%p\n", (void*)&runqueues);

    for (i=0; i<4; i++) {
        struct rq *rq;
        rq = cpu_rq(i);
        printf("i=%d, rq=%p\n", i, (void*)rq);
    }
    return 0;
}
