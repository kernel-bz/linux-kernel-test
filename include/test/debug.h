#ifndef __TEST_DEBUG_H
#define __TEST_DEBUG_H

#include <linux/compiler.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#define pr_info(...)	do { printf("INFO:%s: ", __func__); \
                                printf(__VA_ARGS__); } while (0)

#define pr_info_view(format, args) printf(format, #args, args)

#define pr_fn_start()	printf("--> %s()...\n", __func__)
#define pr_fn_end()		printf("<-- %s().\n", __func__)

#if 0
#define __WARN_printf(arg...)	do { fprintf(stderr, arg); } while (0)

#define WARN(condition, format...) ({		\
    int __ret_warn_on = !!(condition);	\
    if (unlikely(__ret_warn_on))		\
        __WARN_printf(format);		\
    unlikely(__ret_warn_on);		\
})

#define WARN_ON(condition) ({					\
    int __ret_warn_on = !!(condition);			\
    if (unlikely(__ret_warn_on))				\
        __WARN_printf("assertion failed at %s:%d\n",	\
                __FILE__, __LINE__);		\
    unlikely(__ret_warn_on);				\
})

#define WARN_ON_ONCE(condition) ({			\
    static int __warned;				\
    int __ret_warn_once = !!(condition);		\
                            \
    if (unlikely(__ret_warn_once && !__warned)) {	\
        __warned = true;			\
        WARN_ON(1);				\
    }						\
    unlikely(__ret_warn_once);			\
})

#define WARN_ONCE(condition, format...)	({	\
    static int __warned;			\
    int __ret_warn_once = !!(condition);	\
                        \
    if (unlikely(__ret_warn_once))		\
        if (WARN(!__warned, format)) 	\
            __warned = 1;		\
    unlikely(__ret_warn_once);		\
})

#endif

#ifdef __cplusplus
}
#endif

#endif // __TEST_DEBUG_H
