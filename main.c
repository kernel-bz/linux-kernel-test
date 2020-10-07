#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "test/test.h"

int main(void)
{
    cpus_mask_test();
    sched_test();

    return 0;
}
