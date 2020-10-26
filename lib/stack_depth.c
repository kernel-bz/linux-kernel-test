//Refer: https://github.com/zdavkeos/Stack-Depth
//QMAKE_CFLAGS += -finstrument-functions
//Compile with Debug Mode (-g)
#include <stdio.h>

static int total = 0; /* total # of enters/exits */
static int depth = 0; /* current stack depth */
static int levels[100] = { 0 };
// we add the `no_instrument_function'
// so our instrument functions don't
// get instrumented
void __attribute__((no_instrument_function)) __cyg_profile_func_enter(void *this_fn, void *call_site);
void __attribute__((no_instrument_function)) __cyg_profile_func_exit(void *this_fn, void *call_site);

void __cyg_profile_func_enter(void *this_fn, void *call_site)
{
    levels[total++] = ++depth;

    printf("depth=%d\n", depth);
}

void __cyg_profile_func_exit(void *this_fn, void *call_site)
{
    levels[total++] = --depth;

    /* print the results as we exit main... */
    if (depth == 0) {
        int i;

        printf("Total enters/exits: %d\n", total);
        printf("Total function calls: %d\n", total / 2);
        printf("Stack depth level trace:\n");
        for (i = 0; i < total; i++) {
            printf("%d\n", levels[i]);
        }
    }
}
