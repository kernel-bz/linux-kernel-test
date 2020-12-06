/*
 * NUMA support, based on the x86 implementation.
 *
 * Copyright (C) 2015 Cavium Inc.
 * Author: Ganapatrao Kulkarni <gkulkarni@cavium.com>
 */

#define pr_fmt(fmt) "NUMA: " fmt

#include "test/config.h"

//#include <linux/acpi.h>
#include <linux/memblock.h>
#include <linux/module.h>
#include <linux/of.h>

//#include <asm/acpi.h>
#include <asm/sections.h>

#include <linux/numa.h>
#include <linux/export.h>

static int cpu_to_node_map[NR_CPUS] = { [0 ... NR_CPUS-1] = NUMA_NO_NODE };

static int __init early_cpu_to_node(int cpu)
{
        return cpu_to_node_map[cpu];
}

void numa_usr_set_node(int step)
{
    int i;
    int nid = 0, cnt = 0;

    for (i=0; i<NR_CPUS; i++) {
        if (cnt++ == step) {
            nid++;
            cnt = 0;
        }
        cpu_to_node_map[i] = nid;
    }
}
EXPORT_SYMBOL(numa_usr_set_node);

int numa_usr_get_node(int cpu)
{
    return cpu_to_node_map[cpu];
}
EXPORT_SYMBOL(numa_usr_get_node);
