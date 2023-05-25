// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/test.h
 *  Test Functions Header
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#ifndef __TEST_H
#define __TEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/xarray.h>
#include <linux/slub_def.h>

//test/menu/
void menu_basic_train(void);
void menu_algorithm(void);
void menu_config(void);
void menu_start_kernel(void);
void menu_sched_test(void);
void menu_mm_test(void);
void menu_wq_test(void);
void menu_drivers(void);

//test/config/
void config_view(void);
void config_set_debug(void);

//test/algorithm
void list_test01(void);
void list_test02(void);
void list_test03(void);
void list_test04(void);
void list_test05(void);
void lib_list_sort_test(void);
void rbtree_test01(void);
void rbtree_test02(void);
void rbtree_test03(void);
void lib_sort_test(void);

void hlist_test01(void);
void hlist_test02(void);
void hlist_test03(void);
void hlist_test04(void);

void lib_ida_test(void);
void lib_xarray_test(void);
//test/algorithm/xarray/xa-text.c
void xa_debug_xarray_view(struct xarray *xa, u64 index, void *entry);
void xa_debug_state_view(struct xa_state *xas, void *entry);
void xa_debug_node_view(struct xa_node *node, void *entry);
void xa_debug_node_print(struct xarray *xa);
void xa_debug_node_print_range(struct xarray *xa);
void xa_constants_view(void);
void xarray_test_simple(void);
void xarray_test_store_range(void);
void xarray_test_marks(void);
void xarray_test_run(void);
//test/algorithm/xarray/xa-main.c
void xa_main_test(void);
//test/algorithm/xarray/idr-test.c
void idr_ida_main_test(void);
void idr_simple_test(void);
void ida_simple_test(void);
//test/algorithm/xarray/multiorder.c
void xa_multiorder_test(void);

//test/algorithm/mtree/maple.c
int maple_main_test(void);
//test/algorithm/mtree/mt-test.c
void mtree_info(void);
void mtree_basic_store_test(void);
void mtree_basic_store_range_test(void);
void mtree_basic_walk_test(void);
void mtree_store_loop_test(void);
void mtree_store_load_erase_test(void);

//test/init
void test_setup_arch(void);
void test_sched_init(void);
void test_sched_init_smp(void);
void test_numa_init(void);
void test_workqueue_init(void);
void test_rcu_init(void);

//test/sched/sched-test.c
struct task_group *sched_test_tg_select(void);

void test_sched_create_group(void);
void test_sched_new_task(void);
void test_sched_deactivate_task(void);
void test_sched_setscheduler(void);
void test_sched_schedule(void);
void test_sched_wake_up_process(void);
void test_calc_global_load(void);
void test_sched_pelt_info(void);
void test_sched_dl_enqueue(void);
void test_sched_cpudl(void);

void test_sched_core_create(void);
void test_sched_core_to(void);

//test/sched/test-fair.c
void test_fair_set_user_nice(void);
void test_fair_tg_set_cfs_bandwidth(void);
void test_fair_walk_tg_tree_from(struct task_group *from);
void test_fair_walk_tg_tree_from(struct task_group *from);

void test_sched_current_task_info(void);
void test_sched_rq_info(void);

void test_sched_stop_cpu_dying(void);

//test/sched/decay_load.c
void test_decay_load(void);

//test/sched/update_load_avg.c
void test_update_load_avg(void);

//test/sched/sched-pelt.c
void sched_pelt_constants(void);

//test/memory/mm-test.c
int mm_memblock_add(unsigned int size);
void mm_memblock_test(void);
void mm_constant_infos(void);
void mm_kmem_cache_slub_info(struct kmem_cache_slub *s);

//kernel/sched/fair.c
void sched_fair_run_rebalance(void);
void sched_fair_vruntime_test(void);
void sched_fair_task_tick_test(void);

//kernel/sched/rt.c
void rt_debug_rt_rq_info(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_H
