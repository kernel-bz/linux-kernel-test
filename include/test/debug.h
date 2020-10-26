#ifndef __TEST_DEBUG_H
#define __TEST_DEBUG_H

#include <linux/compiler.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int DebugLevel;
extern int DebugBase;
//if (DebugLevel > 0) debug on
//if (level>=DebugBase && level<=DebugLevel) debug on
//depth = level - DebugBase
//if (depth > 0) debug on

//#define printk		printf
#define seq_printf		printf
#define pr_warning		printf
#define pr_debug		printf
#define printk(...) 	dprintf(STDOUT_FILENO, __VA_ARGS__)
#define pr_err(format, ...) do { fprintf (stderr, format, ## __VA_ARGS__); } while (0)
#define pr_warn 		pr_err
#define pr_cont 		pr_err
#define panic 			pr_err
#define print_tainted() ""

#define pr_out(...)							\
    do {									\
      if (DebugLevel > 0) 					\
        printf(__VA_ARGS__); 				\
    } while (0)

#define pr_info(...)						\
    do {									\
      if (DebugLevel > 0) {					\
        printf("FUNC : %s : ", __func__); 	\
        printf(__VA_ARGS__); 				\
      }										\
    } while (0)

#define pr_info_view(format, args) 			\
    do {									\
      if (DebugLevel > 0) 					\
        printf(format, #args, args);		\
    } while (0)

#define pr_fn_start()						\
    do {									\
      if (DebugLevel > 0) 					\
        printf("--> %s()...\n", __func__);	\
    } while (0)

#define pr_fn_end()							\
    do {									\
      if (DebugLevel > 0) 					\
        printf("<-- %s().\n\n", __func__);	\
    } while (0)

#define pr_out_on(level, ...)						\
    do {											\
      int depth = level - DebugBase;				\
      if (depth > 0) {								\
        if (level > DebugLevel) break;				\
        while (depth--) { printf("    "); }			\
        printf(__VA_ARGS__); 						\
      }												\
    } while (0)

#define pr_info_on(level, ...)						\
    do {											\
      int depth = level - DebugBase;				\
      if (depth > 0) {								\
        if (level > DebugLevel) break;				\
        while (depth--) { printf("    "); }			\
        printf("<%d> : %s : ", level, __func__); 	\
        printf(__VA_ARGS__); 						\
      }												\
    } while (0)

#define pr_info_view_on(level, format, args)		\
    do {											\
      int depth = level - DebugBase;				\
      if (depth > 0) {								\
        if (level > DebugLevel) break;				\
        while (depth--) { printf("    "); }			\
        printf("<%d> : ", level); 					\
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


#ifdef __cplusplus
}
#endif

#endif // __TEST_DEBUG_H
