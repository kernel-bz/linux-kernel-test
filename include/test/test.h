#ifndef __TEST_H
#define __TEST_H

#ifdef __cplusplus
extern "C" {
#endif

//test/algorithm
void struct_algorithm(void);
int list_test01(void);
int list_test02(void);
int list_test03(void);
int list_test04(void);
int rbtree_test01(void);
int rbtree_test02(void);
int rbtree_test03(void);

void init_start_kernel(void);

//test/sched/sched-test.c
void sched_test(void);
struct task_group *sched_test_tg_select(void);

//test/sched/decay_load.c
void decay_load_test(void);

//test/sched/update_load_avg.c
void update_load_avg_test(void);

//kernel/sched/fair.c
void sched_fair_run_rebalance(void);


#ifdef __cplusplus
}
#endif

#endif // TEST_H
