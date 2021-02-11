/*
 * NUMA support, based on the x86 implementation.
 *
 * Copyright (C) 2015 Cavium Inc.
 * Author: Ganapatrao Kulkarni <gkulkarni@cavium.com>
 */

#define pr_fmt(fmt) "NUMA: " fmt

#include "test/config.h"
#include "test/debug.h"

//#include <linux/acpi.h>
#include <linux/memblock.h>
#include <linux/module.h>
#include <linux/of.h>

//#include <asm/acpi.h>
#include <asm/sections.h>

//#include <linux/numa.h>
#include <linux/nodemask.h>
#include <linux/cpumask.h>
#include <linux/export.h>
#include <arch/arm64/include/asm/numa_.h>

nodemask_t numa_nodes_parsed __initdata;
static int cpu_to_node_map[NR_CPUS] = { [0 ... NR_CPUS-1] = NUMA_NO_NODE };

static int numa_distance_cnt;
static u8 *numa_distance;
bool numa_off;

struct cpumask __node_to_cpumask_map[MAX_NUMNODES];

cpumask_var_t node_to_cpumask_map[MAX_NUMNODES];
EXPORT_SYMBOL(node_to_cpumask_map);

#ifdef CONFIG_DEBUG_PER_CPU_MAPS

/*
 * Returns a pointer to the bitmask of CPUs on Node 'node'.
 */
const struct cpumask *cpumask_of_node(int node)
{
    if (WARN_ON(node >= nr_node_ids))
        return cpu_none_mask;

    if (WARN_ON(node_to_cpumask_map[node] == NULL))
        return cpu_online_mask;

    return node_to_cpumask_map[node];
}
EXPORT_SYMBOL(cpumask_of_node);

#endif

static void __init setup_node_to_cpumask_map(void)
{
        int node;

        /* setup nr_node_ids if not done yet */
        //if (nr_node_ids == MAX_NUMNODES)
        //        setup_nr_node_ids();

        /* allocate and clear the mapping */
        for (node = 0; node < nr_node_ids; node++) {
                //alloc_bootmem_cpumask_var(&node_to_cpumask_map[node]);
                node_to_cpumask_map[node] = &__node_to_cpumask_map[node];
                cpumask_clear(node_to_cpumask_map[node]);
        }

        /* cpumask_of_node() will now work */
        pr_debug("Node to cpumask map for %u nodes\n", nr_node_ids);
}


static void numa_update_cpu(unsigned int cpu, bool remove)
{
        int nid = cpu_to_node(cpu);

        if (nid == NUMA_NO_NODE)
                return;

        if (remove)
                cpumask_clear_cpu(cpu, node_to_cpumask_map[nid]);
        else
                cpumask_set_cpu(cpu, node_to_cpumask_map[nid]);
}

static void numa_usr_set_cpumask_map(void)
{
    pr_fn_start_on(stack_depth);

    unsigned int cpu;
    for (cpu=0; cpu<NR_CPUS; cpu++) {
        numa_update_cpu(cpu, false);
    }

    cpumask_var_t cpumask;
    int nid;
    for (cpu=0; cpu<NR_CPUS; cpu++) {
        nid = cpu_to_node(cpu);
        cpumask = cpumask_of_node(nid);
        pr_view_on(stack_depth, "%20s : %u\n", cpu);
        pr_view_on(stack_depth, "%20s : %d\n", nid);
        pr_view_on(stack_depth, "%20s : 0x%X\n", cpumask->bits[0]);
    }

    pr_fn_end_on(stack_depth);
}


void numa_usr_set_node(int step)
{
    pr_fn_start_on(stack_depth);

    int i;
    int nid = 0;

    pr_view_on(stack_depth, "%20s : %d\n", step);

    for (i=0; i<NR_CPUS; i++) {
        nid = i / step;
        cpu_to_node_map[i] = nid;
        pr_view_on(stack_depth, "%20s : %d\n", i);
        pr_view_on(stack_depth, "%20s : %d\n", nid);
    }

    pr_fn_end_on(stack_depth);
}
EXPORT_SYMBOL(numa_usr_set_node);

int numa_usr_get_node(int cpu)
{
    return cpu_to_node_map[cpu];
}
EXPORT_SYMBOL(numa_usr_get_node);



//200 lines
int __init numa_add_memblk(int nid, u64 start, u64 end)
{
    int ret;

    ret = memblock_set_node(start, (end - start), &memblock.memory, nid);
    if (ret < 0) {
        pr_err("memblock [0x%llx - 0x%llx] failed to add on node %d\n",
            start, (end - 1), nid);
        return ret;
    }

    node_set(nid, numa_nodes_parsed);
    return ret;
}



//268 lines
/*
 * Create a new NUMA distance table.
 */
static int __init numa_alloc_distance(void)
{
    size_t size;
    u64 phys;
    int i, j;

    size = nr_node_ids * nr_node_ids * sizeof(numa_distance[0]);
    numa_distance = kzalloc(size, GFP_KERNEL);
    if (WARN_ON(!numa_distance))
        return -ENOMEM;

    numa_distance_cnt = nr_node_ids;

    /* fill with the default distances */
    for (i = 0; i < numa_distance_cnt; i++)
        for (j = 0; j < numa_distance_cnt; j++)
            numa_distance[i * numa_distance_cnt + j] = i == j ?
                LOCAL_DISTANCE : REMOTE_DISTANCE;

    pr_debug("Initialized distance table, cnt=%d\n", numa_distance_cnt);

    return 0;
}
//299
/**
 * numa_set_distance() - Set inter node NUMA distance from node to node.
 * @from: the 'from' node to set distance
 * @to: the 'to'  node to set distance
 * @distance: NUMA distance
 *
 * Set the distance from node @from to @to to @distance.
 * If distance table doesn't exist, a warning is printed.
 *
 * If @from or @to is higher than the highest known node or lower than zero
 * or @distance doesn't make sense, the call is ignored.
 */
void __init numa_set_distance(int from, int to, int distance)
{
    if (!numa_distance) {
        pr_warn_once("Warning: distance table not allocated yet\n");
        return;
    }

    if (from >= numa_distance_cnt || to >= numa_distance_cnt ||
            from < 0 || to < 0) {
        pr_warn_once("Warning: node ids are out of bound, from=%d to=%d distance=%d\n",
                from, to, distance);
        return;
    }

    if ((u8)distance != distance ||
        (from == to && distance != LOCAL_DISTANCE)) {
        pr_warn_once("Warning: invalid distance parameter, from=%d to=%d distance=%d\n",
                 from, to, distance);
        return;
    }

    numa_distance[from * numa_distance_cnt + to] = distance;
}
//335
/*
 * Return NUMA distance @from to @to
 */
int __node_distance(int from, int to)
{
    if (from >= numa_distance_cnt || to >= numa_distance_cnt)
        return from == to ? LOCAL_DISTANCE : REMOTE_DISTANCE;
    return numa_distance[from * numa_distance_cnt + to];
}
EXPORT_SYMBOL(__node_distance);

/**
 * arm64_numa_init() - Initialize NUMA
 *
 * Try each configured NUMA initialization method until one succeeds. The
 * last fallback is dummy single node config encomapssing whole memory.
 */
void __init arm64_numa_init(void)
{
    pr_fn_start_on(stack_depth);

    //nodes_clear(numa_nodes_parsed);
    //nodes_clear(node_possible_map);
    //nodes_clear(node_online_map);

    numa_alloc_distance();
    numa_set_distance(0, 2, RECLAIM_DISTANCE);
    numa_set_distance(1, 3, RECLAIM_DISTANCE);
    numa_set_distance(2, 0, RECLAIM_DISTANCE);
    numa_set_distance(3, 1, RECLAIM_DISTANCE);

    setup_node_to_cpumask_map();
    numa_usr_set_cpumask_map();

    pr_fn_end_on(stack_depth);
}

void numa_distance_info(void)
{
    pr_fn_start_on(stack_depth);

    int i, j;
    u8 node_dist;

    //numa_distance_cnt = nr_node_ids;

    for (i = 0; i < numa_distance_cnt; i++) {
        printf("%d: ", i);
        for (j = 0; j < numa_distance_cnt; j++) {
            node_dist = numa_distance[i * numa_distance_cnt + j];
            printf("%u, ", node_dist);
        }
        printf("\n");
    }
    pr_fn_end_on(stack_depth);
}

//mm/page_alloc.c
/*
 * Array of node states.
 */
nodemask_t node_states[NR_NODE_STATES] __read_mostly = {
    [N_POSSIBLE] = NODE_MASK_ALL,
    [N_ONLINE] = { { [0] = 1UL } },
#ifndef CONFIG_NUMA
    [N_NORMAL_MEMORY] = { { [0] = 1UL } },
#ifdef CONFIG_HIGHMEM
    [N_HIGH_MEMORY] = { { [0] = 1UL } },
#endif
    [N_MEMORY] = { { [0] = 1UL } },
    [N_CPU] = { { [0] = 1UL } },
#endif	/* NUMA */
};
EXPORT_SYMBOL(node_states);
