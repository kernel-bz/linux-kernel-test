#ifndef _LINUX_MM_H
#define _LINUX_MM_H

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>

//typedef unsigned long dma_addr_t;

#define PAGE_SIZE	(4096)
#define PAGE_SHIFT	(12)
#define PAGE_MASK	(~(PAGE_SIZE-1))

#define __ALIGN_KERNEL(x, a)		__ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_KERNEL_MASK(x, mask)	(((x) + (mask)) & ~(mask))
#define ALIGN(x, a)			__ALIGN_KERNEL((x), (a))

#define PAGE_ALIGN(addr) ALIGN(addr, PAGE_SIZE)

#define offset_in_page(p)	((unsigned long)(p) & ~PAGE_MASK)

//include/asm-generic/page.h
//#define virt_to_page(x)	((void *)x)
#define page_address(x)	((void *)x)

static inline unsigned long page_to_phys(struct page *page)
{
	assert(0);

	return 0;
}

#define page_to_pfn(page) ((unsigned long)(page) / PAGE_SIZE)
#define pfn_to_page(pfn) (void *)((pfn) * PAGE_SIZE)
#define nth_page(page,n) pfn_to_page(page_to_pfn((page)) + (n))

#define __min(t1, t2, min1, min2, x, y) ({              \
	t1 min1 = (x);                                  \
	t2 min2 = (y);                                  \
	(void) (&min1 == &min2);                        \
	min1 < min2 ? min1 : min2; })

#define ___PASTE(a,b) a##b
#define __PASTE(a,b) ___PASTE(a,b)

#define __UNIQUE_ID(prefix) __PASTE(__PASTE(__UNIQUE_ID_, prefix), __COUNTER__)

#define min(x, y)                                       \
	__min(typeof(x), typeof(y),                     \
	      __UNIQUE_ID(min1_), __UNIQUE_ID(min2_),   \
	      x, y)

#define min_t(type, x, y)                               \
	__min(type, type,                               \
	      __UNIQUE_ID(min1_), __UNIQUE_ID(min2_),   \
	      x, y)

static inline void *kmap(struct page *page)
{
	assert(0);

	return NULL;
}

static inline void *kmap_atomic(struct page *page)
{
	assert(0);

	return NULL;
}

static inline void kunmap(void *addr)
{
	assert(0);
}

static inline void kunmap_atomic(void *addr)
{
	assert(0);
}

static inline unsigned long __get_free_page(unsigned int flags)
{
	return (unsigned long)malloc(PAGE_SIZE);
}

static inline void free_page(unsigned long page)
{
	free((void *)page);
}

#define kmemleak_alloc(a, b, c, d)
#define kmemleak_free(a)

#define PageSlab(p) (0)
#define flush_kernel_dcache_page(p)

//623 lines
enum {
    REGION_INTERSECTS,
    REGION_DISJOINT,
    REGION_MIXED,
};



#endif	//_LINUX_MM_H
