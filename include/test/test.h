#ifndef __TEST_H
#define __TEST_H

#ifdef __cplusplus
extern "C" {
#endif

//test/cpu/cpus-mask-test.c
int cpus_mask_test(void);

//test/sched/decay_load.c
int decay_load_test(void);

//test/sched/update_load_avg.c
int update_load_avg_test(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_H
