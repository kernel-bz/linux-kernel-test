#ifndef __TEST_H
#define __TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/sched.h>

//test/cpu/cpus-mask-test.c
void cpus_mask_test(void);

//test/sched/sched-test.c
void sched_test(void);

//test/sched/decay_load.c
void decay_load_test(void);

//test/sched/update_load_avg.c
void update_load_avg_test(void);

//kernel/sched/fair.c
void enqueue_task_fair_test(struct task_struct *p);


#ifdef __cplusplus
}
#endif

#endif // TEST_H
