/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __TEST_USER_TYPES_H
#define __TEST_USER_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define __SANE_USERSPACE_TYPES__	/* For PPC64, to get LL64 types */

#include <asm/types.h>
#include <asm/posix_types.h>
//#include <linux/types.h>
#include <linux/compiler.h>
#include <asm-generic/int-ll64.h>
#include <uapi/linux/limits.h>
#include <linux/limits.h>

#include <bits/types/__sigset_t.h>

//usr/include/x86_64-linux-gnu/asm/signal.h
//#include <asm/signal.h>

#if defined(__aarch64__)
//usr/include/aarch64-linux-gnu/bits/types/sigset_t.h
#include <bits/types/sigset_t.h>
#endif

//include/linux/sysfs.h
#define loff_t	__loff_t

//include/linux/types.h
/* bsd */
typedef unsigned char 	u_char;
typedef unsigned short 	u_short;
typedef unsigned int 	u_int;
typedef unsigned long 	u_long;

/* sysv */
typedef unsigned char		unchar;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef unsigned long		ulong;

//fs/ntfs/types.h
typedef __le16 le16;
typedef __le32 le32;
typedef __le64 le64;
typedef __u16 __bitwise sle16;
typedef __u32 __bitwise sle32;
typedef __u64 __bitwise sle64;


typedef unsigned int __kernel_dev_t;

//typedef __kernel_fd_set		fd_set;
//typedef __kernel_dev_t		dev_t;
typedef __kernel_ino_t			ino_t;
//typedef __kernel_mode_t		mode_t;		//sys/types.h
typedef unsigned short			umode_t;
//typedef u32					nlink_t;
//typedef __kernel_off_t		off_t;
typedef __kernel_pid_t			pid_t;
typedef __kernel_daddr_t		daddr_t;
//typedef __kernel_key_t		key_t;
typedef __kernel_suseconds_t	suseconds_t;
//typedef __kernel_timer_t		timer_t;
typedef __kernel_clockid_t		clockid_t;
//typedef __kernel_mqd_t		mqd_t;

//stdbool.h
//typedef _Bool			bool;

typedef __kernel_uid32_t		uid_t;
typedef __kernel_gid32_t		gid_t;
typedef __kernel_uid16_t        uid16_t;
typedef __kernel_gid16_t        gid16_t;

//typedef unsigned long			uintptr_t;

#ifdef CONFIG_HAVE_UID16
/* This is defined by include/asm-{arch}/posix_types.h */
typedef __kernel_old_uid_t	old_uid_t;
typedef __kernel_old_gid_t	old_gid_t;
#endif /* CONFIG_UID16 */

struct kmem_cache;
struct page;

//unistd.h
//typedef unsigned long        size_t;
//typedef   signed long       ssize_t;

#ifndef _TIME_T
#define _TIME_T
typedef __kernel_time_t		time_t;
#endif

#ifndef _CLOCK_T
#define _CLOCK_T
typedef __kernel_clock_t	clock_t;
#endif


#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
typedef u64 dma_addr_t;
#else
typedef u32 dma_addr_t;
#endif

#ifdef __CHECKER__
#define __bitwise__ __attribute__((bitwise))
#else
#define __bitwise__
#endif

#define __bitwise __bitwise__

typedef unsigned int __bitwise gfp_t;
typedef unsigned int __bitwise slab_flags_t;
typedef unsigned int __bitwise fmode_t;

#ifdef CONFIG_PHYS_ADDR_T_64BIT
typedef u64 phys_addr_t;
#else
typedef u32 phys_addr_t;
#endif

typedef phys_addr_t resource_size_t;

/*
 * This type is the placeholder for a hardware interrupt number. It has to be
 * big enough to enclose whatever representation is used by a given platform.
 */
typedef unsigned long irq_hw_number_t;


typedef struct {
	int counter;
} atomic_t;

#ifndef __aligned_u64
# define __aligned_u64 __u64 __attribute__((aligned(8)))
#endif

struct list_head {
	struct list_head *next, *prev;
};

struct hlist_head {
	struct hlist_node *first;
};

struct hlist_node {
	struct hlist_node *next, **pprev;
};


struct ustat {
        __kernel_daddr_t        f_tfree;
        __kernel_ino_t          f_tinode;
        char                    f_fname[6];
        char                    f_fpack[6];
};

/**
 * struct callback_head - callback structure for use with RCU and task_work
 * @next: next update requests in a list
 * @func: actual update function to call after the grace period.
 *
 * The struct is aligned to size of pointer. On most architectures it happens
 * naturally due ABI requirements, but some architectures (like CRIS) have
 * weird ABI and we need to ask it explicitly.
 *
 * The alignment is required to guarantee that bit 0 of @next will be
 * clear under normal conditions -- as long as we use call_rcu() or
 * call_srcu() to queue the callback.
 *
 * This guarantee is important for few reasons:
 *  - future call_rcu_lazy() will make use of lower bits in the pointer;
 *  - the structure shares storage space in struct page with @compound_head,
 *    which encode PageTail() in bit 0. The guarantee is needed to avoid
 *    false-positive PageTail().
 */
struct callback_head {
        struct callback_head *next;
        void (*func)(struct callback_head *head);
} __attribute__((aligned(sizeof(void *))));
//#define rcu_head callback_head

typedef void (*rcu_callback_t)(struct rcu_head *head);
typedef void (*call_rcu_func_t)(struct rcu_head *head, rcu_callback_t func);

#ifdef __cplusplus
}
#endif

#endif /* __TEST_USER_TYPES_H */
