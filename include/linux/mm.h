#ifndef _LINUX_MM_H
#define _LINUX_MM_H

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include <asm-generic/page.h>

//arch/*/include/asm/pgtable-types.h
typedef unsigned long   pteval_t;
typedef unsigned long   pmdval_t;
typedef unsigned long   pudval_t;
typedef unsigned long   p4dval_t;
typedef unsigned long   pgdval_t;
typedef unsigned long   pgprotval_t;

//typedef unsigned long dma_addr_t;

//arch/*/include/asm/page_types.h
#define PAGE_SHIFT	(12)
//#define PAGE_SIZE	(4096)
#define PAGE_SIZE   (_AC(1,UL) << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))

#define __ALIGN_KERNEL(x, a)		__ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_KERNEL_MASK(x, mask)	(((x) + (mask)) & ~(mask))
#define ALIGN(x, a)			__ALIGN_KERNEL((x), (a))

#define PAGE_ALIGN(addr) ALIGN(addr, PAGE_SIZE)

#define offset_in_page(p)	((unsigned long)(p) & ~PAGE_MASK)

//include/asm-generic/page.h
//#define virt_to_page(x)	((void *)x)
#define page_address(x)	((void *)x)

//180 lines
/*
 * Default maximum number of active map areas, this limits the number of vmas
 * per mm struct. Users can overwrite this number by sysctl but there is a
 * problem.
 *
 * When a program's coredump is generated as ELF format, a section is created
 * per a vma. In ELF, the number of sections is represented in unsigned short.
 * This means the number of sections should be smaller than 65535 at coredump.
 * Because the kernel adds some informative sections to a image of program at
 * generating coredump, we need some margin. The number of extra sections is
 * 1-3 now and depends on arch. We use "5" as safe margin, here.
 *
 * ELF extended numbering allows more than 65535 sections, so 16-bit bound is
 * not a hard limit any more. Although some userspace tools can be surprised by
 * that.
 */
#define MAPCOUNT_ELF_CORE_MARGIN        (5)
#define DEFAULT_MAX_MAP_COUNT   (USHRT_MAX - MAPCOUNT_ELF_CORE_MARGIN)


/*
 * vm_flags in vm_area_struct, see mm_types.h.
 * When changing, update also include/trace/events/mmflags.h
 */
#define VM_NONE         0x00000000

#define VM_READ         0x00000001      /* currently active flags */
#define VM_WRITE        0x00000002
#define VM_EXEC         0x00000004
#define VM_SHARED       0x00000008

/* mprotect() hardcodes VM_MAYREAD >> 4 == VM_READ, and so for r/w/x bits. */
#define VM_MAYREAD      0x00000010      /* limits for mprotect() etc */
#define VM_MAYWRITE     0x00000020
#define VM_MAYEXEC      0x00000040
#define VM_MAYSHARE     0x00000080

#define VM_GROWSDOWN    0x00000100      /* general info on the segment */
#define VM_UFFD_MISSING 0x00000200      /* missing pages tracking */
#define VM_PFNMAP       0x00000400      /* Page-ranges managed without "struct page", just pure PFN */
#define VM_DENYWRITE    0x00000800      /* ETXTBSY on write attempts.. */
#define VM_UFFD_WP      0x00001000      /* wrprotect pages tracking */

#define VM_LOCKED       0x00002000
#define VM_IO           0x00004000      /* Memory mapped I/O or similar */
                                                                                                                     /* Used by sys_madvise() */
#define VM_SEQ_READ     0x00008000      /* App will access data sequentially */
#define VM_RAND_READ    0x00010000      /* App will not benefit from clustered reads */

#define VM_DONTCOPY     0x00020000      /* Do not copy this vma on fork */
#define VM_DONTEXPAND   0x00040000      /* Cannot expand with mremap() */
#define VM_LOCKONFAULT  0x00080000      /* Lock the pages covered when they are faulted in */
#define VM_ACCOUNT      0x00100000      /* Is a VM accounted object */
#define VM_NORESERVE    0x00200000      /* should the VM suppress accounting */
#define VM_HUGETLB      0x00400000      /* Huge TLB Page VM */
#define VM_SYNC         0x00800000      /* Synchronous page faults */
#define VM_ARCH_1       0x01000000      /* Architecture-specific flag */
#define VM_WIPEONFORK   0x02000000      /* Wipe VMA contents in child. */
#define VM_DONTDUMP     0x04000000      /* Do not include in the core dump */

#ifdef CONFIG_MEM_SOFT_DIRTY
# define VM_SOFTDIRTY   0x08000000      /* Not soft dirty clean area */
#else
# define VM_SOFTDIRTY   0
#endif

#define VM_MIXEDMAP     0x10000000      /* Can contain "struct page" and pure PFN pages */
#define VM_HUGEPAGE     0x20000000      /* MADV_HUGEPAGE marked this vma */
#define VM_NOHUGEPAGE   0x40000000      /* MADV_NOHUGEPAGE marked this vma */
#define VM_MERGEABLE    0x80000000      /* KSM may merge identical pages */

#ifdef CONFIG_ARCH_USES_HIGH_VMA_FLAGS
#define VM_HIGH_ARCH_BIT_0      32      /* bit only usable on 64-bit architectures */
#define VM_HIGH_ARCH_BIT_1      33      /* bit only usable on 64-bit architectures */
#define VM_HIGH_ARCH_BIT_2      34      /* bit only usable on 64-bit architectures */
#define VM_HIGH_ARCH_BIT_3      35      /* bit only usable on 64-bit architectures */
#define VM_HIGH_ARCH_BIT_4      36      /* bit only usable on 64-bit architectures */
#define VM_HIGH_ARCH_0  BIT(VM_HIGH_ARCH_BIT_0)
#define VM_HIGH_ARCH_1  BIT(VM_HIGH_ARCH_BIT_1)
#define VM_HIGH_ARCH_2  BIT(VM_HIGH_ARCH_BIT_2)
#define VM_HIGH_ARCH_3  BIT(VM_HIGH_ARCH_BIT_3)
#define VM_HIGH_ARCH_4  BIT(VM_HIGH_ARCH_BIT_4)
#endif /* CONFIG_ARCH_USES_HIGH_VMA_FLAGS */

#ifdef CONFIG_ARCH_HAS_PKEYS
# define VM_PKEY_SHIFT  VM_HIGH_ARCH_BIT_0
# define VM_PKEY_BIT0   VM_HIGH_ARCH_0  /* A protection key is a 4-bit value */
# define VM_PKEY_BIT1   VM_HIGH_ARCH_1  /* on x86 and 5-bit value on ppc64   */
# define VM_PKEY_BIT2   VM_HIGH_ARCH_2
# define VM_PKEY_BIT3   VM_HIGH_ARCH_3
#ifdef CONFIG_PPC
# define VM_PKEY_BIT4  VM_HIGH_ARCH_4
#else
# define VM_PKEY_BIT4  0
#endif
#endif /* CONFIG_ARCH_HAS_PKEYS */

#if defined(CONFIG_X86)
# define VM_PAT         VM_ARCH_1       /* PAT reserves whole VMA at once (x86) */
#elif defined(CONFIG_PPC)
# define VM_SAO         VM_ARCH_1       /* Strong Access Ordering (powerpc) */
#elif defined(CONFIG_PARISC)
# define VM_GROWSUP     VM_ARCH_1
#elif defined(CONFIG_IA64)
# define VM_GROWSUP     VM_ARCH_1
#elif defined(CONFIG_SPARC64)
# define VM_SPARC_ADI   VM_ARCH_1       /* Uses ADI tag for access control */
# define VM_ARCH_CLEAR  VM_SPARC_ADI
#elif !defined(CONFIG_MMU)
# define VM_MAPPED_COPY VM_ARCH_1       /* T if mapped copy of data (nommu mmap) */
#endif

#if defined(CONFIG_X86_INTEL_MPX)
/* MPX specific bounds table or bounds directory */
# define VM_MPX         VM_HIGH_ARCH_4
#else
# define VM_MPX         VM_NONE
#endif

#ifndef VM_GROWSUP
# define VM_GROWSUP     VM_NONE
#endif

/* Bits set in the VMA until the stack is in its final location */
#define VM_STACK_INCOMPLETE_SETUP       (VM_RAND_READ | VM_SEQ_READ)

#ifndef VM_STACK_DEFAULT_FLAGS          /* arch can override this */
#define VM_STACK_DEFAULT_FLAGS VM_DATA_DEFAULT_FLAGS
#endif

#ifdef CONFIG_STACK_GROWSUP
#define VM_STACK        VM_GROWSUP
#else
#define VM_STACK        VM_GROWSDOWN
#endif
#define VM_STACK_FLAGS  (VM_STACK | VM_STACK_DEFAULT_FLAGS | VM_ACCOUNT)

/*
 * Special vmas that are non-mergable, non-mlock()able.
 * Note: mm/huge_memory.c VM_NO_THP depends on this definition.
 */
#define VM_SPECIAL (VM_IO | VM_DONTEXPAND | VM_PFNMAP | VM_MIXEDMAP)

/* This mask defines which mm->def_flags a process can inherit its parent */
#define VM_INIT_DEF_MASK        VM_NOHUGEPAGE

/* This mask is used to clear all the VMA flags used by mlock */
#define VM_LOCKED_CLEAR_MASK    (~(VM_LOCKED | VM_LOCKONFAULT))

/* Arch-specific flags to clear when updating VM flags on protection change */
#ifndef VM_ARCH_CLEAR
# define VM_ARCH_CLEAR  VM_NONE
#endif
#define VM_FLAGS_CLEAR  (ARCH_VM_PKEY_FLAGS | VM_ARCH_CLEAR)

/*
 * mapping from the currently active vm_flags protection bits (the
 * low four bits) to a page protection mask..
 */
extern pgprot_t protection_map[16];

#define FAULT_FLAG_WRITE        0x01    /* Fault was a write access */
#define FAULT_FLAG_MKWRITE      0x02    /* Fault was mkwrite of existing pte */
#define FAULT_FLAG_ALLOW_RETRY  0x04    /* Retry fault if blocking */
#define FAULT_FLAG_RETRY_NOWAIT 0x08    /* Don't drop mmap_sem and wait when retrying */
#define FAULT_FLAG_KILLABLE     0x10    /* The fault task is in SIGKILL killable region */
#define FAULT_FLAG_TRIED        0x20    /* Second try */
#define FAULT_FLAG_USER         0x40    /* The fault originated in userspace */
#define FAULT_FLAG_REMOTE       0x80    /* faulting for non current tsk/mm */
#define FAULT_FLAG_INSTRUCTION  0x100   /* The fault was during an instruction fetch */

/*
static inline unsigned long page_to_phys(struct page *page)
{
	assert(0);

	return 0;
}
*/

#define offset_in_page(p)       ((unsigned long)(p) & ~PAGE_MASK)


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
