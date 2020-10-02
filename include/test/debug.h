#ifndef __TEST_DEBUG_H
#define __TEST_DEBUG_H

#include <linux/compiler.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define printk			printf
#define seq_printf		printf
#define pr_warning		printf
#define pr_cont			printf
#define pr_debug		printf

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



#ifdef __cplusplus
}
#endif

#endif // __TEST_DEBUG_H
