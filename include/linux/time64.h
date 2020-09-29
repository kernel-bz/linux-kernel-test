/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _TOOLS_LINUX_TIME64_H
#define _TOOLS_LINUX_TIME64_H

#define MSEC_PER_SEC	1000L
#define USEC_PER_MSEC	1000L
#define NSEC_PER_USEC	1000L
#define NSEC_PER_MSEC	1000000L
#define USEC_PER_SEC	1000000L
#define NSEC_PER_SEC	1000000000L
#define FSEC_PER_SEC	1000000000000000LL


#include <linux/math64.h>
//#include <vdso/time64.h>

typedef __s64 time64_t;
typedef __u64 timeu64_t;

//#include <uapi/linux/time.h>

struct timespec64 {
    time64_t	tv_sec;			/* seconds */
    long		tv_nsec;		/* nanoseconds */
};

struct itimerspec64 {
    struct timespec64 it_interval;
    struct timespec64 it_value;
};

/* Located here for timespec[64]_valid_strict */
#define TIME64_MAX			((s64)~((u64)1 << 63))
#define TIME64_MIN			(-TIME64_MAX - 1)

#define KTIME_MAX			((s64)~((u64)1 << 63))
#define KTIME_SEC_MAX			(KTIME_MAX / NSEC_PER_SEC)


#endif /* _LINUX_TIME64_H */
