//Refer: https://github.com/zdavkeos/Stack-Depth
//QMAKE_CFLAGS += -finstrument-functions
//Compile with Debug Mode (-g)
#include <stdio.h>

int stack_depth = -2;
// we add the `no_instrument_function'
// so our instrument functions don't get instrumented
void __attribute__((no_instrument_function))
    __cyg_profile_func_enter(void *this_fn, void *call_site);
void __attribute__((no_instrument_function))
    __cyg_profile_func_exit(void *this_fn, void *call_site);

void __cyg_profile_func_enter(void *this_fn, void *call_site)
{
    stack_depth++;
    //printf("stack_depth=%d, %p\n", stack_depth, this_fn);
}

void __cyg_profile_func_exit(void *this_fn, void *call_site)
{
    stack_depth--;
    //printf("stack_depth=%d, %p\n", stack_depth, this_fn);
}
