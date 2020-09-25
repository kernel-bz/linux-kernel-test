#include <stdio.h>
#include <stdlib.h>

#define __KERNEL__

#define NR_CPUS		4

//include/asm-generic/bitsperlong.h
#ifdef CONFIG_64BIT
#define BITS_PER_LONG 64
#else
#define BITS_PER_LONG 32
#endif /* CONFIG_64BIT */

#ifndef BITS_PER_LONG_LONG
#define BITS_PER_LONG_LONG 64
#endif

//include/uapi/linux/kernel.h
#define __KERNEL_DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#define DIV_ROUND_UP __KERNEL_DIV_ROUND_UP

//include/linux/bitops.h
#ifdef  __KERNEL__
#define BIT(nr)                 (1UL << (nr))
#define BIT_ULL(nr)             (1ULL << (nr))
#define BIT_MASK(nr)            (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)            ((nr) / BITS_PER_LONG)
#define BIT_ULL_MASK(nr)        (1ULL << ((nr) % BITS_PER_LONG_LONG))
#define BIT_ULL_WORD(nr)        ((nr) / BITS_PER_LONG_LONG)
#define BITS_PER_BYTE           8
#define BITS_TO_LONGS(nr)       DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))
#endif

#define DECLARE_BITMAP(name,bits) \
        unsigned long name[BITS_TO_LONGS(bits)]
/*
#define set_bit(nr,p)                   ATOMIC_BITOP(set_bit,nr,p)
#define clear_bit(nr,p)                 ATOMIC_BITOP(clear_bit,nr,p)
#define change_bit(nr,p)                ATOMIC_BITOP(change_bit,nr,p)
#define test_and_set_bit(nr,p)          ATOMIC_BITOP(test_and_set_bit,nr,p)
#define test_and_clear_bit(nr,p)        ATOMIC_BITOP(test_and_clear_bit,nr,p)
#define test_and_change_bit(nr,p)       ATOMIC_BITOP(test_and_change_bit,nr,p)
*/

//tools/include/asm-generic/bitops/atomic.h
static inline void set_bit(unsigned int nr, unsigned long *addr)
{
    addr[nr / BITS_PER_LONG] |= 1UL << (nr % BITS_PER_LONG);
}

static inline void clear_bit(unsigned int nr, unsigned long *addr)
{
    addr[nr / BITS_PER_LONG] &= ~(1UL << (nr % BITS_PER_LONG));
}

//include/linux/cpumask.h
typedef struct cpumask { DECLARE_BITMAP(bits, NR_CPUS); } cpumask_t;
typedef struct cpumask *cpumask_var_t;

#define cpumask_bits(maskp) ((maskp)->bits)

#ifdef CONFIG_CPUMASK_OFFSTACK
#define nr_cpumask_bits nr_cpu_ids
#else
#define nr_cpumask_bits ((unsigned int)NR_CPUS)
#endif

static inline unsigned int cpumask_check(unsigned int cpu)
{
    if (cpu >= nr_cpumask_bits)
        printf("WARN_ON_ONCE(): cpu >= nr_cpumask_bits\n");
    return cpu;
}

static inline void cpumask_set_cpu(unsigned int cpu, struct cpumask *dstp)
{
    set_bit(cpumask_check(cpu), cpumask_bits(dstp));
}

static inline void cpumask_clear_cpu(unsigned int cpu, struct cpumask *dstp)
{
    clear_bit(cpumask_check(cpu), cpumask_bits(dstp));
}

static inline size_t cpumask_size(void)
{
        return BITS_TO_LONGS(nr_cpumask_bits) * sizeof(long);
}

int cpus_mask_test(void)
{
    unsigned int bits;
    for (bits=0; bits<=128; bits+=8)
        printf("BITS_TO_LONGS bits=%u, nr=%u\n", bits, BITS_TO_LONGS(bits));

    unsigned int i, nr = BITS_TO_LONGS(nr_cpumask_bits);
    cpumask_var_t cpus_mask;
    cpus_mask = calloc(1, cpumask_size());

    for (i=0; i<nr; i++)
        printf("cpus_mask->bits = %lu\n", cpus_mask->bits[i]);

    unsigned int cpu;
    for (cpu=0; cpu<nr_cpumask_bits; cpu++) {
        cpumask_set_cpu(cpu, cpus_mask);
        for (i=0; i<nr; i++)
            printf("cpu=%d, cpus_mask->bits=%lu\n", cpu, cpus_mask->bits[i]);
        cpumask_clear_cpu(cpu, cpus_mask);
    }
    return 0;
}
