/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _PERF_BITOPS_H
#define _PERF_BITOPS_H

#include <stdio.h>
#include <stdlib.h>

#include <linux/bitops.h>
#include <linux/kernel.h>


extern int __bitmap_equal(const unsigned long *bitmap1,
              const unsigned long *bitmap2, unsigned int nbits);
extern int __bitmap_andnot(unsigned long *dst, const unsigned long *bitmap1,
            const unsigned long *bitmap2, unsigned int nbits);
extern int __bitmap_intersects(const unsigned long *bitmap1,
            const unsigned long *bitmap2, unsigned int nbits);
extern int __bitmap_subset(const unsigned long *bitmap1,
            const unsigned long *bitmap2, unsigned int nbits);

#define DECLARE_BITMAP(name,bits) \
	unsigned long name[BITS_TO_LONGS(bits)]

int __bitmap_weight(const unsigned long *bitmap, int bits);
void __bitmap_or(unsigned long *dst, const unsigned long *bitmap1,
		 const unsigned long *bitmap2, int bits);
int __bitmap_and(unsigned long *dst, const unsigned long *bitmap1,
         const unsigned long *bitmap2, unsigned int bits);void bitmap_clear(unsigned long *map, unsigned int start, int len);

#define BITMAP_FIRST_WORD_MASK(start) (~0UL << ((start) & (BITS_PER_LONG - 1)))

#define BITMAP_LAST_WORD_MASK(nbits)					\
(									\
	((nbits) % BITS_PER_LONG) ?					\
		(1UL<<((nbits) % BITS_PER_LONG))-1 : ~0UL		\
)

#define small_const_nbits(nbits) \
	(__builtin_constant_p(nbits) && (nbits) <= BITS_PER_LONG)

static inline void bitmap_zero(unsigned long *dst, int nbits)
{
    if (small_const_nbits(nbits))
        *dst = 0UL;
    else {
        int len = BITS_TO_LONGS(nbits) * sizeof(unsigned long);
        memset(dst, 0, len);
    }
}

static inline void bitmap_fill(unsigned long *dst, unsigned int nbits)
{
    unsigned int nlongs = BITS_TO_LONGS(nbits);

    if (!small_const_nbits(nbits)) {
        unsigned int len = (nlongs - 1) * sizeof(unsigned long);
        memset(dst, 0xff,  len);
    }
    //printf("mask(nbits) = 0x%X\n", BITMAP_LAST_WORD_MASK(nbits));
    dst[nlongs - 1] = BITMAP_LAST_WORD_MASK(nbits);
}

static inline int bitmap_empty(const unsigned long *src, unsigned nbits)
{
    if (small_const_nbits(nbits))
        return ! (*src & BITMAP_LAST_WORD_MASK(nbits));

    return find_first_bit(src, nbits) == nbits;
}

static inline int bitmap_full(const unsigned long *src, unsigned int nbits)
{
    if (small_const_nbits(nbits))
        return ! (~(*src) & BITMAP_LAST_WORD_MASK(nbits));

    return find_first_zero_bit(src, nbits) == nbits;
}

static inline int bitmap_weight(const unsigned long *src, int nbits)
{
    if (small_const_nbits(nbits))
        return hweight_long(*src & BITMAP_LAST_WORD_MASK(nbits));
    return __bitmap_weight(src, nbits);
}

static inline void bitmap_or(unsigned long *dst, const unsigned long *src1,
			     const unsigned long *src2, int nbits)
{
	if (small_const_nbits(nbits))
		*dst = *src1 | *src2;
	else
		__bitmap_or(dst, src1, src2, nbits);
}

/**
 * test_and_set_bit - Set a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 */
static inline int test_and_set_bit(int nr, unsigned long *addr)
{
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long old;

	old = *p;
	*p = old | mask;

	return (old & mask) != 0;
}

/**
 * test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to clear
 * @addr: Address to count from
 */
static inline int test_and_clear_bit(int nr, unsigned long *addr)
{
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long old;

	old = *p;
	*p = old & ~mask;

	return (old & mask) != 0;
}

/**
 * bitmap_alloc - Allocate bitmap
 * @nbits: Number of bits
 */
static inline unsigned long *bitmap_alloc(int nbits)
{
	return calloc(1, BITS_TO_LONGS(nbits) * sizeof(unsigned long));
}

/*
 * bitmap_scnprintf - print bitmap list into buffer
 * @bitmap: bitmap
 * @nbits: size of bitmap
 * @buf: buffer to store output
 * @size: size of @buf
 */
size_t bitmap_scnprintf(unsigned long *bitmap, int nbits,
			char *buf, size_t size);

/**
 * bitmap_and - Do logical and on bitmaps
 * @dst: resulting bitmap
 * @src1: operand 1
 * @src2: operand 2
 * @nbits: size of bitmap
 */
static inline int bitmap_and(unsigned long *dst, const unsigned long *src1,
			     const unsigned long *src2, unsigned int nbits)
{
	if (small_const_nbits(nbits))
		return (*dst = *src1 & *src2 & BITMAP_LAST_WORD_MASK(nbits)) != 0;
	return __bitmap_and(dst, src1, src2, nbits);
}



//210 lines
/*
 * The static inlines below do not handle constant nbits==0 correctly,
 * so make such users (should any ever turn up) call the out-of-line
 * versions.
 */
#define small_const_nbits(nbits) \
    (__builtin_constant_p(nbits) && (nbits) <= BITS_PER_LONG && (nbits) > 0)

static inline void bitmap_copy(unsigned long *dst, const unsigned long *src,
            unsigned int nbits)
{
    unsigned int len = BITS_TO_LONGS(nbits) * sizeof(unsigned long);
    memcpy(dst, src, len);
}
//237 lines



//292 lines
static inline int bitmap_andnot(unsigned long *dst, const unsigned long *src1,
            const unsigned long *src2, unsigned int nbits)
{
    if (small_const_nbits(nbits))
        return (*dst = *src1 & ~(*src2) & BITMAP_LAST_WORD_MASK(nbits)) != 0;
    return __bitmap_andnot(dst, src1, src2, nbits);
}


//309 lines
#ifdef __LITTLE_ENDIAN
#define BITMAP_MEM_ALIGNMENT 8
#else
#define BITMAP_MEM_ALIGNMENT (8 * sizeof(unsigned long))
#endif
#define BITMAP_MEM_MASK (BITMAP_MEM_ALIGNMENT - 1)

static inline int bitmap_equal(const unsigned long *src1,
            const unsigned long *src2, unsigned int nbits)
{
    if (small_const_nbits(nbits))
        return !((*src1 ^ *src2) & BITMAP_LAST_WORD_MASK(nbits));
    if (__builtin_constant_p(nbits & BITMAP_MEM_MASK) &&
        IS_ALIGNED(nbits, BITMAP_MEM_ALIGNMENT))
        return !memcmp(src1, src2, nbits / 8);
    return __bitmap_equal(src1, src2, nbits);
}
//327
/**
 * bitmap_or_equal - Check whether the or of two bitmaps is equal to a third
 * @src1:	Pointer to bitmap 1
 * @src2:	Pointer to bitmap 2 will be or'ed with bitmap 1
 * @src3:	Pointer to bitmap 3. Compare to the result of *@src1 | *@src2
 * @nbits:	number of bits in each of these bitmaps
 *
 * Returns: True if (*@src1 | *@src2) == *@src3, false otherwise
 */
static inline bool bitmap_or_equal(const unsigned long *src1,
                   const unsigned long *src2,
                   const unsigned long *src3,
                   unsigned int nbits)
{
    if (!small_const_nbits(nbits))
        return __bitmap_or_equal(src1, src2, src3, nbits);

    return !(((*src1 | *src2) ^ *src3) & BITMAP_LAST_WORD_MASK(nbits));
}

static inline int bitmap_intersects(const unsigned long *src1,
            const unsigned long *src2, unsigned int nbits)
{
    if (small_const_nbits(nbits))
        return ((*src1 & *src2) & BITMAP_LAST_WORD_MASK(nbits)) != 0;
    else
        return __bitmap_intersects(src1, src2, nbits);
}
//356
static inline int bitmap_subset(const unsigned long *src1,
            const unsigned long *src2, unsigned int nbits)
{
    if (small_const_nbits(nbits))
        return ! ((*src1 & ~(*src2)) & BITMAP_LAST_WORD_MASK(nbits));
    else
        return __bitmap_subset(src1, src2, nbits);
}


#endif /* _PERF_BITOPS_H */
