
// SPDX-License-Identifier: GPL-2.0
/*
 * CPU subsystem support
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/cpu.h>
#include <linux/topology.h>
#include <linux/device.h>
//#include <linux/node.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/percpu.h>
//#include <linux/acpi.h>
#include <linux/of.h>
//#include <linux/cpufeature.h>
//#include <linux/tick.h>
//#include <linux/pm_qos.h>
#include <linux/sched/isolation.h>

//#include <linux/export.h>

#include "base.h"

static DEFINE_PER_CPU(struct device *, cpu_sys_devices);

static int cpu_subsys_match(struct device *dev, struct device_driver *drv)
{
    /* ACPI style match is the only one that may succeed. */
    //if (acpi_driver_match_device(dev, drv))
        //return 1;

    return 0;
}




//397 lines
struct device *get_cpu_device(unsigned cpu)
{
    if (cpu < nr_cpu_ids && cpu_possible(cpu))
        return per_cpu(cpu_sys_devices, cpu);
    else
        return NULL;
}
//EXPORT_SYMBOL_GPL(get_cpu_device);

static void device_create_release(struct device *dev)
{
    kfree(dev);
}


