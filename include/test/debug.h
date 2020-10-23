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
//depth = level - base
//if (depth >= 0) debug on
//output depth is level

//#define printk			printf
#define seq_printf		printf
#define pr_warning		printf
#define pr_debug		printf
#define printk(...) 	dprintf(STDOUT_FILENO, __VA_ARGS__)
#define pr_err(format, ...) do { fprintf (stderr, format, ## __VA_ARGS__); } while (0)
#define pr_warn 		pr_err
#define pr_cont 		pr_err
#define panic 			pr_err
#define print_tainted() ""

#define pr_out			printf

#define pr_out_on(base, ...)				\
    do {if (DebugLevel >= base) { 			\
            pr_out(__VA_ARGS__);			\
        }} while (0)

#define pr_info(...)						\
    do {									\
      int depth = DebugLevel - DebugBase;	\
      if (depth > 0) { 						\
        while (depth--) { printf("    "); }	\
        printf("FUNC : %s : ", __func__); 	\
        printf(__VA_ARGS__); 				\
    }} while (0)

#define pr_info_view(format, args) 			\
    do {									\
      int depth = DebugLevel - DebugBase;	\
      if (depth > 0) { 						\
        while (depth--) { printf("    "); }	\
        printf(format, #args, args);		\
    }} while (0)

#define pr_fn_start()						\
    do {									\
      int depth = DebugLevel - DebugBase;	\
      if (depth > 0) { 						\
        while (depth--) { printf("    "); }	\
        printf("--> %s()...\n", __func__);	\
    }} while (0)

#define pr_fn_end()							\
    do {									\
      int depth = DebugLevel - DebugBase;	\
      if (depth > 0) { 						\
        while (depth--) { printf("    "); }	\
        printf("<-- %s().\n\n", __func__);	\
    }} while (0)

#define pr_info_on(base, ...)				\
    do {									\
      int depth = DebugLevel - base;		\
      if (depth >= 0) { 					\
        while (base--) { printf("    "); }	\
        printf("<%d> : %s : ", base, __func__); 	\
        printf(__VA_ARGS__); 				\
    }} while (0)

#define pr_info_view_on(base, format, args)	\
   do {										\
      int depth = DebugLevel - base;		\
      if (depth >= 0) { 					\
        depth = base;						\
        while (depth--) { printf("    "); }	\
        printf(format, #args, args);		\
    }} while (0)

#define pr_fn_start_on(base)				\
    do {									\
      int depth = DebugLevel - base;		\
      if (depth >= 0) { 					\
        depth = base;						\
        while (depth--) { printf("    "); }	\
        printf("%d--> %s()...\n", base, __func__);	\
    }} while (0)

#define pr_fn_end_on(base)					\
    do {									\
      int depth = DebugLevel - base;		\
      if (depth >= 0) { 					\
        depth = base;						\
        while (depth--) { printf("    "); }	\
        printf("%d<-- %s().\n\n", base, __func__);	\
    }} while (0)


#ifdef __cplusplus
}
#endif

#endif // __TEST_DEBUG_H
