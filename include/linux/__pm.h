/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 *  pm.h - Power management interface
 *
 *  Copyright (C) 2000 Andrew Henroid
 */

#ifndef _LINUX_PM_H
#define _LINUX_PM_H

#include "test/define-usr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pm_message {
    int event;
} pm_message_t;

struct pm_subsys_data {
    unsigned int refcount;
};

struct dev_pm_info {
    pm_message_t	power_state;
    unsigned int	can_wakeup:1;
    unsigned int	async_suspend:1;
    bool			in_dpm_list:1;
    bool			is_prepared:1;
    bool			is_suspended:1;
    bool			is_noirq_suspended:1;
    bool			is_late_suspended:1;
    bool			no_pm:1;
    bool			early_init:1;
    bool			direct_complete:1;
    u32				driver_flags;
    struct pm_subsys_data *subsys_data;
};


#ifdef __cplusplus
}
#endif

#endif // _LINUX_PM_H
