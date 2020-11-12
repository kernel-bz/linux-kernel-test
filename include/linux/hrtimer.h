#ifndef _LINUX_HRTIMER_H
#define _LINUX_HRTIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/ktime.h>

struct hrtimer_clock_base;
struct hrtimer_cpu_base;

enum hrtimer_mode {
    HRTIMER_MODE_ABS	= 0x00,
    HRTIMER_MODE_REL	= 0x01,
    HRTIMER_MODE_PINNED	= 0x02,
    HRTIMER_MODE_SOFT	= 0x04,
    HRTIMER_MODE_HARD	= 0x08,

    HRTIMER_MODE_ABS_PINNED = HRTIMER_MODE_ABS | HRTIMER_MODE_PINNED,
    HRTIMER_MODE_REL_PINNED = HRTIMER_MODE_REL | HRTIMER_MODE_PINNED,

    HRTIMER_MODE_ABS_SOFT	= HRTIMER_MODE_ABS | HRTIMER_MODE_SOFT,
    HRTIMER_MODE_REL_SOFT	= HRTIMER_MODE_REL | HRTIMER_MODE_SOFT,

    HRTIMER_MODE_ABS_PINNED_SOFT = HRTIMER_MODE_ABS_PINNED | HRTIMER_MODE_SOFT,
    HRTIMER_MODE_REL_PINNED_SOFT = HRTIMER_MODE_REL_PINNED | HRTIMER_MODE_SOFT,

    HRTIMER_MODE_ABS_HARD	= HRTIMER_MODE_ABS | HRTIMER_MODE_HARD,
    HRTIMER_MODE_REL_HARD	= HRTIMER_MODE_REL | HRTIMER_MODE_HARD,

    HRTIMER_MODE_ABS_PINNED_HARD = HRTIMER_MODE_ABS_PINNED | HRTIMER_MODE_HARD,
    HRTIMER_MODE_REL_PINNED_HARD = HRTIMER_MODE_REL_PINNED | HRTIMER_MODE_HARD,
};

/*
 * Return values for the callback function
 */
enum hrtimer_restart {
    HRTIMER_NORESTART,	/* Timer is not restarted */
    HRTIMER_RESTART,	/* Timer must be restarted */
};

#define HRTIMER_STATE_INACTIVE	0x00
#define HRTIMER_STATE_ENQUEUED	0x01

struct hrtimer {
    //struct timerqueue_node		node;
    ktime_t				_softexpires;
    enum hrtimer_restart		(*function)(struct hrtimer *);
    struct hrtimer_clock_base	*base;
    u8				state;
    u8				is_rel;
    u8				is_soft;
    u8				is_hard;
};

struct hrtimer_sleeper;

#ifdef CONFIG_64BIT
# define __hrtimer_clock_base_align	____cacheline_aligned
#else
# define __hrtimer_clock_base_align
#endif


struct hrtimer_clock_base;

enum  hrtimer_base_type {
    HRTIMER_BASE_MONOTONIC,
    HRTIMER_BASE_REALTIME,
    HRTIMER_BASE_BOOTTIME,
    HRTIMER_BASE_TAI,
    HRTIMER_BASE_MONOTONIC_SOFT,
    HRTIMER_BASE_REALTIME_SOFT,
    HRTIMER_BASE_BOOTTIME_SOFT,
    HRTIMER_BASE_TAI_SOFT,
    HRTIMER_MAX_CLOCK_BASES,
};

struct hrtimer_cpu_base;

static void hrtimer_init(struct hrtimer *timer, clockid_t which_clock,
                  enum hrtimer_mode mode) { }
static bool hrtimer_active(const struct hrtimer *timer) { }
static inline void hrtimer_start(struct hrtimer *timer, ktime_t tim,
                                 const enum hrtimer_mode mode) { }

static inline int hrtimer_is_queued(struct hrtimer *timer) { }
static inline ktime_t hrtimer_cb_get_time(struct hrtimer *timer) { }

static int hrtimer_cancel(struct hrtimer *timer) { }
static int hrtimer_try_to_cancel(struct hrtimer *timer) { }

static inline u64 hrtimer_forward_now(struct hrtimer *timer,
                                      ktime_t interval) { }

static inline void hrtimer_start_expires(struct hrtimer *timer,
                                         enum hrtimer_mode mode) { }


#ifdef __cplusplus
}
#endif

#endif
