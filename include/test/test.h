#ifndef __TEST_H
#define __TEST_H

#ifdef __cplusplus
extern "C" {
#endif

//test/menu/
void menu_basic_train(void);
void menu_algorithm(void);
void menu_config(void);
void menu_init(void);
void menu_sched_test(void);

//test/config/
void config_view(void);
void config_set_debug_level(void);

//test/basic/
void basic_struct_test(void);
void basic_ptr_test(void);

//test/algorithm
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
void test_setup_arch(void);
void test_sched_init(void);
void test_sched_init_smp(void);
void test_numa_init(void);

//test/sched/sched-test.c
void test_sched_create_group(void);
void test_sched_new_task(void);
void test_sched_current_task_info(void);
void test_sched_deactivate_task(void);
void test_sched_setscheduler(void);
void test_sched_schedule(void);
void test_sched_wake_up_process(void);
void test_calc_global_load(void);
void test_sched_pelt_info(void);
void test_sched_set_user_nice(void);
void test_sched_dl_enqueue(void);
void test_sched_cpudl(void);

//test/sched/decay_load.c
void test_decay_load(void);

//test/sched/update_load_avg.c
void test_update_load_avg(void);

//kernel/sched/fair.c
void sched_fair_run_rebalance(void);


#ifdef __cplusplus
}
#endif

#endif // TEST_H
