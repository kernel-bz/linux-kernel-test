#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "test/test.h"
#include "linux/types.h"

int main(void)
{
    cpus_mask_test();

    decay_load_test();

    update_load_avg_test();

    return 0;
}
