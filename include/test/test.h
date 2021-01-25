#ifndef __TEST_H
#define __TEST_H

#ifdef __cplusplus
extern "C" {
#endif

//test/config/
void config_setting(void);

//test/basic/
void basic_struct_test(void);
void basic_ptr_test(void);

//test/algorithm
void algorithm_struct(void);
void list_test01(void);
void list_test02(void);
void list_test03(void);
void list_test04(void);
void lib_list_sort_test(void);

void rbtree_test01(void);
void rbtree_test02(void);
void rbtree_test03(void);

void lib_sort_test(void);

void lib_ida_test(void);
void lib_xarray_test(void);

//test/init
void init_start_kernel(void);
void test_setup_arch(void);
void test_sched_init(void);
void test_sched_init_smp(void);
void test_numa_init(void);

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
