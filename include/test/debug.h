#ifndef __TEST_DEBUG_H
#define __TEST_DEBUG_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/compiler.h>
#include <linux/sched.h>
#include <linux/build_bug.h>
#include <asm-generic/bug.h>

#ifdef __cplusplus
extern "C" {
#endif

//main.c
extern int DebugLevel;
extern int DebugBase;
//if (DebugLevel > 0) debug on
//if (level > DebugBase && level <= DebugLevel) debug on
//depth = level - DebugBase
//if (depth > 0) debug on

//lib/stack_depth.c
extern int stack_depth;

#define printk			printf
#define seq_printf		printf
#define pr_warning		_pr_warn
#define pr_debug		printf
#define printk(...) 	dprintf(STDOUT_FILENO, __VA_ARGS__)
#define pr_warn 				_pr_warn
#define pr_warn_once			_pr_warn
#define pr_cont 				pr_err
#define panic 					pr_err
#define print_tainted() 		""
#define printk_deferred_once	_pr_warn
#define pr_crit					pr_err


#define pr_err(format, ...)							\
    do { 											\
        fprintf(stderr, "ERROR | %s : ", __func__);	\
        fprintf(stderr, format, ## __VA_ARGS__); 	\
    } while (0)

#define _pr_warn(format, ...)						\
    do { 											\
        fprintf(stderr, "WARNING | %s : ", __func__);	\
        fprintf(stderr, format, ## __VA_ARGS__); 	\
    } while (0)

#define pr_info(...)						\
    do {									\
      if (DebugLevel > 0) {					\
        printf("FUNC | %s : ", __func__); 	\
        printf(__VA_ARGS__); 				\
      }										\
    } while (0)

#define pr_out_on(level, ...)						\
    do {											\
      int depth = level - DebugBase;				\
      if (depth > 0) {								\
        if (level > DebugLevel) break;				\
        while (depth--) { printf("    "); }			\
        printf("<%d> | ", level); 					\
        printf(__VA_ARGS__); 						\
      }												\
    } while (0)

#define pr_info_on(level, ...)						\
    do {											\
      int depth = level - DebugBase;				\
      if (depth > 0) {								\
        if (level > DebugLevel) break;				\
        while (depth--) { printf("    "); }			\
        printf("<%d> | %s : ", level, __func__); 	\
        printf(__VA_ARGS__); 						\
      }												\
    } while (0)

#define pr_info_view_on(level, format, args)		\
    do {											\
      int depth = level - DebugBase;				\
      if (depth > 0) {								\
        if (level > DebugLevel) break;				\
        while (depth--) { printf("    "); }			\
        printf("<%d> | ", level); 					\
        printf(format, #args, args);				\
      }												\
    } while (0)

#define pr_fn_start_on(level)						\
    do {											\
      int depth = level - DebugBase;				\
      if (depth > 0) {								\
        if (level > DebugLevel) break;				\
        while (depth--) { printf("    "); }			\
        printf("%d--> %s()...\n", level, __func__);	\
      }												\
    } while (0)

#define pr_fn_end_on(level)							\
    do {											\
      int depth = level - DebugBase;				\
      if (depth > 0) {								\
        if (level > DebugLevel) break;				\
        while (depth--) { printf("    "); }			\
        printf("%d<-- %s().\n\n", level, __func__);	\
      }												\
    } while (0)


//test/sched/sched-test.c
extern struct task_struct *current_task;

void pr_sched_avg_info(struct sched_avg *sa);
void pr_sched_pelt_info(struct sched_entity *se);

void pr_sched_tg_view_cpu(struct task_group *tg);
int pr_sched_tg_view_only(void);

void pr_sched_tg_info_all(void);
void pr_sched_tg_info(void);

void pr_leaf_cfs_rq_info(void);
void pr_cfs_rq_removed_info(struct cfs_rq *cfs_rq);

void pr_sched_dl_entity_info(struct sched_dl_entity *dl_se);
void pr_sched_curr_task_info(struct task_struct *p);

void pr_sched_cpumask_bits_info(unsigned int nr_cpu);

#ifdef __cplusplus
}
#endif

#endif // __TEST_DEBUG_H
