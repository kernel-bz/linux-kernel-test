/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_LINUX_TIME_TYPES_H
#define _UAPI_LINUX_TIME_TYPES_H

#include <uapi/asm-generic/posix_types.h>
#include <linux/types.h>

struct __kernel_timespec_ {
    //__kernel_time64_t       tv_sec;                 /* seconds */
    long long		        tv_sec;                 /* seconds */
    long long               tv_nsec;                /* nanoseconds */
};

struct __kernel_itimerspec_ {
    struct __kernel_timespec_ it_interval;    /* timer period */
    struct __kernel_timespec_ it_value;       /* timer expiration */
};

/*
 * legacy timeval structure, only embedded in structures that
 * traditionally used 'timeval' to pass time intervals (not absolute
 * times). Do not add new users. If user space fails to compile
 * here, this is probably because it is not y2038 safe and needs to
 * be changed to use another interface.
 */
#ifndef __kernel_old_timeval_
struct __kernel_old_timeval_ {
	__kernel_long_t tv_sec;
	__kernel_long_t tv_usec;
};
#endif

struct __kernel_sock_timeval_ {
	__s64 tv_sec;
	__s64 tv_usec;
};

#endif /* _UAPI_LINUX_TIME_TYPES_H */
