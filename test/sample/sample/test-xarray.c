#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

#define CONFIG_64BIT
#ifdef CONFIG_64BIT
#define BITS_PER_LONG 64
#else
#define BITS_PER_LONG 32
#endif /* CONFIG_64BIT */

#define BIT_ULL(nr)             (ULL(1) << (nr))
#define BIT_MASK(nr)            (UL(1) << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)            ((nr) / BITS_PER_LONG)
#define BIT_ULL_MASK(nr)        (ULL(1) << ((nr) % BITS_PER_LONG_LONG))
#define BIT_ULL_WORD(nr)        ((nr) / BITS_PER_LONG_LONG)
#define BITS_PER_BYTE           8

#define CONFIG_BASE_SMALL		1

#define XA_CHUNK_SHIFT			2
#ifndef XA_CHUNK_SHIFT
#define XA_CHUNK_SHIFT          (CONFIG_BASE_SMALL ? 4 : 6)
#endif
#define XA_CHUNK_SIZE           (1UL << XA_CHUNK_SHIFT)
#define XA_CHUNK_MASK           (XA_CHUNK_SIZE - 1)
#define XA_MAX_MARKS            3
#define XA_MARK_LONGS           DIV_ROUND_UP(XA_CHUNK_SIZE, BITS_PER_LONG)

typedef unsigned int gfp_t;
typedef unsigned xa_mark_t;
//#define __GFP_BITS_SHIFT (25 + IS_ENABLED(CONFIG_LOCKDEP))
#define __GFP_BITS_SHIFT 		25
#define __GFP_BITS_MASK ((gfp_t)((1 << __GFP_BITS_SHIFT) - 1))

#define XA_MARK_0               ((xa_mark_t)0U)
#define XA_MARK_1               ((xa_mark_t)1U)
#define XA_MARK_2               ((xa_mark_t)2U)
#define XA_PRESENT              ((xa_mark_t)8U)
#define XA_MARK_MAX             XA_MARK_2
#define XA_FREE_MARK            XA_MARK_0

#define XA_FLAGS_TRACK_FREE     ((gfp_t)4U)
#define XA_FLAGS_ZERO_BUSY      ((gfp_t)8U)
#define XA_FLAGS_ALLOC_WRAPPED  ((gfp_t)16U)
#define XA_FLAGS_ACCOUNT        ((gfp_t)32U)
#define XA_FLAGS_MARK(mark)     ((gfp_t)((1U << __GFP_BITS_SHIFT) << \
                                                (unsigned)(mark)))

struct xa_node {
    union {
        unsigned long tags[XA_MAX_MARKS][XA_MARK_LONGS];
        unsigned long marks[XA_MAX_MARKS][XA_MARK_LONGS];
    };
};

struct xarray {
    gfp_t		xa_flags;
    void 		*xa_head;
};

#define XARRAY_INIT(name, flags) {		\
    .xa_flags = flags,					\
    .xa_head = NULL,					\
}

//Find first bit from LSB
static unsigned long __ffs(unsigned long word)
{
    int num = 0;

#if BITS_PER_LONG == 64
    if ((word & 0xffffffff) == 0) {
        num += 32;
        word >>= 32;
    }
#endif
    if ((word & 0xffff) == 0) {
        num += 16;
        word >>= 16;
    }
    if ((word & 0xff) == 0) {
        num += 8;
        word >>= 8;
    }
    if ((word & 0xf) == 0) {
        num += 4;
        word >>= 4;
    }
    if ((word & 0x3) == 0) {
        num += 2;
        word >>= 2;
    }
    if ((word & 0x1) == 0)
        num += 1;
    return num;
}

/**
 * test_bit - Determine whether a bit is set
 * @nr: bit number to test
 * @addr: Address to start counting from
 */
static inline int test_bit(int nr, const volatile unsigned long *addr)
{
    return 1UL & (addr[BIT_WORD(nr)] >> (nr & (BITS_PER_LONG-1)));
}

static inline bool xa_marked(const struct xarray *xa, xa_mark_t mark)
{
    return xa->xa_flags & XA_FLAGS_MARK(mark);
}

#define BITMAP_LAST_WORD_MASK(nbits)					\
(									\
    ((nbits) % BITS_PER_LONG) ?					\
        (1UL<<((nbits) % BITS_PER_LONG))-1 : ~0UL		\
)

//CONFIG_XARRAY_MULTI
static int _main_set_order(int index, int order)
{
    int shift, sibs, index2;
    printf("index=%d, order=%d\n", index, order);
    index2 = order < BITS_PER_LONG ? (index >> order) << order : 0;
    shift = order - (order % XA_CHUNK_SHIFT);
    sibs = (1U << (order % XA_CHUNK_SHIFT)) - 1;
    printf("index=%d, shift=%d, sibs=%d\n\n", index2, shift, sibs);
    //return (index - index2);
    return 0;
}

struct test_s {
        char a;
        int b;
};
static struct test_s ts = {
        .a = 10,
        .b = 20,
};

typedef struct maple_enode *maple_enode; /* encoded node */
typedef struct maple_pnode *maple_pnode; /* parent node */


int main(void)
{
    printf("XA_CHUNK_SHIFT:		%d\n", XA_CHUNK_SHIFT);
    printf("XA_CHUNK_SIZE:		%lu\n", XA_CHUNK_SIZE);
    printf("XA_CHUNK_MASK:		0x%lX\n", XA_CHUNK_MASK);
    printf("XA_MAX_MARKS:		%d\n", XA_MAX_MARKS);
    printf("XA_MARK_LONGS:		%lu\n", XA_MARK_LONGS);
    printf("BITS_PER_LONG-1:	0x%X\n", BITS_PER_LONG-1);

    int offset;

    for (offset = 1; offset < 128; offset += 16)
        printf("%d: BIT_WORD: %d\n", offset, BIT_WORD(offset));

    for (offset = 1; offset < 128; offset += 16) {
        printf("mod: %d\n", offset % BITS_PER_LONG);
        printf("%d: BITMAP_LAST_WORD_MASK: 0x%lX\n", offset, BITMAP_LAST_WORD_MASK(offset));
    }

    unsigned long order;
    for (offset = 0; offset < 20; offset++) {
        order = __ffs(offset+1);
        printf("word:0x%X, order: 0x%lX\n", offset, order);
    }

    for (offset = 0; offset < BITS_PER_LONG; offset++) {
        order = __ffs(offset+1);
        if (_main_set_order(offset, order)) break;
    }

    printf("ts=%p\n", ts);
    printf("ts=%p\n", ts.a);
    printf("ts=%d\n", ts.a);
    printf("&ts=%p\n", &ts);

    //struct maple_pnode *pn;
    //maple_pnode pn;
    //printf("size=%d\n", sizeof(*pn));


    return 0;
}
