#include <stdio.h>

#include "test/debug.h"

#include <linux/types-user.h>
#include <linux/math64.h>
#include "kernel/sched/sched-pelt.h"

//kernel/sched/sched-pelt.h
/*
static const u32 runnable_avg_yN_inv[] __maybe_unused = {
        0xffffffff, 0xfa83b2da, 0xf5257d14, 0xefe4b99a, 0xeac0c6e6, 0xe5b906e6,
        0xe0ccdeeb, 0xdbfbb796, 0xd744fcc9, 0xd2a81d91, 0xce248c14, 0xc9b9bd85,
        0xc5672a10, 0xc12c4cc9, 0xbd08a39e, 0xb8fbaf46, 0xb504f333, 0xb123f581,
        0xad583ee9, 0xa9a15ab4, 0xa5fed6a9, 0xa2704302, 0x9ef5325f, 0x9b8d39b9,
        0x9837f050, 0x94f4efa8, 0x91c3d373, 0x8ea4398a, 0x8b95c1e3, 0x88980e80,
        0x85aac367, 0x82cd8698,
};

#define LOAD_AVG_PERIOD 		32
#define LOAD_AVG_MAX 		 47742
*/

//kernel/sched/pelt.c
static u64 decay_load(u64 val, u64 n)
{
        unsigned int local_n;

        if (n > LOAD_AVG_PERIOD * 63)
                return 0;

        /* after bounds checking we can collapse to 32-bit */
        local_n = n;

        /*
         * As y^PERIOD = 1/2, we can combine
         *    y^n = 1/2^(n/PERIOD) * y^(n%PERIOD)
         * With a look-up table which covers y^n (n<PERIOD)
         *
         * To achieve constant time decay_load.
         */
        if (local_n >= LOAD_AVG_PERIOD) {
                val >>= local_n / LOAD_AVG_PERIOD;
                local_n %= LOAD_AVG_PERIOD;
        }

        val = mul_u64_u32_shr(val, runnable_avg_yN_inv[local_n], 32);
        return val;
}

void decay_load_test(void)
{
    //u64 val=LOAD_AVG_MAX, n;
    u64 val=1024, sum=0, n;

    pr_fn_start();

    for (n=1; n<360; n++) {
        val = decay_load(1024, n);
        sum += val;
        printf("n=%llu : val=%llu, sum=%llu\n", n, val, sum);
    }

    pr_fn_end();
}
