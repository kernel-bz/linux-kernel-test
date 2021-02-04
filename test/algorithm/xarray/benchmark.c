// SPDX-License-Identifier: GPL-2.0-only
/*
 * benchmark.c:
 * Author: Konstantin Khlebnikov <koct9i@gmail.com>
 */
#include <urcu.h>
#include <urcu/urcu-memb.h>

#include <linux/radix-tree.h>
#include <linux/radix-tree-user.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <time.h>
#include <linux/time.h>
#include <linux/time64.h>
#include "xa-test.h"

#define NSEC_PER_SEC	1000000000L

static long long benchmark_iter(struct radix_tree_root *root, bool tagged)
{
	volatile unsigned long sink = 0;
	struct radix_tree_iter iter;
	struct timespec start, finish;
	long long nsec;
	int l, loops = 1;
	void **slot;

#ifdef BENCHMARK
again:
#endif
	clock_gettime(CLOCK_MONOTONIC, &start);
	for (l = 0; l < loops; l++) {
		if (tagged) {
			radix_tree_for_each_tagged(slot, root, &iter, 0, 0)
				sink ^= (unsigned long)slot;
		} else {
			radix_tree_for_each_slot(slot, root, &iter, 0)
				sink ^= (unsigned long)slot;
		}
	}
	clock_gettime(CLOCK_MONOTONIC, &finish);

	nsec = (finish.tv_sec - start.tv_sec) * NSEC_PER_SEC +
	       (finish.tv_nsec - start.tv_nsec);

#ifdef BENCHMARK
	if (loops == 1 && nsec * 5 < NSEC_PER_SEC) {
		loops = NSEC_PER_SEC / nsec / 4 + 1;
		goto again;
	}
#endif

	nsec /= loops;
	return nsec;
}

static void benchmark_insert(struct radix_tree_root *root,
			     unsigned long size, unsigned long step)
{
	struct timespec start, finish;
	unsigned long index;
	long long nsec;

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (index = 0 ; index < size ; index += step)
		item_insert(root, index);

	clock_gettime(CLOCK_MONOTONIC, &finish);

	nsec = (finish.tv_sec - start.tv_sec) * NSEC_PER_SEC +
	       (finish.tv_nsec - start.tv_nsec);

	printv(2, "Size: %8ld, step: %8ld, insertion: %15lld ns\n",
		size, step, nsec);
}

static void benchmark_tagging(struct radix_tree_root *root,
			     unsigned long size, unsigned long step)
{
	struct timespec start, finish;
	unsigned long index;
	long long nsec;

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (index = 0 ; index < size ; index += step)
		radix_tree_tag_set(root, index, 0);

	clock_gettime(CLOCK_MONOTONIC, &finish);

	nsec = (finish.tv_sec - start.tv_sec) * NSEC_PER_SEC +
	       (finish.tv_nsec - start.tv_nsec);

	printv(2, "Size: %8ld, step: %8ld, tagging: %17lld ns\n",
		size, step, nsec);
}

static void benchmark_delete(struct radix_tree_root *root,
			     unsigned long size, unsigned long step)
{
	struct timespec start, finish;
	unsigned long index;
	long long nsec;

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (index = 0 ; index < size ; index += step)
		item_delete(root, index);

	clock_gettime(CLOCK_MONOTONIC, &finish);

	nsec = (finish.tv_sec - start.tv_sec) * NSEC_PER_SEC +
	       (finish.tv_nsec - start.tv_nsec);

	printv(2, "Size: %8ld, step: %8ld, deletion: %16lld ns\n",
		size, step, nsec);
}

static void benchmark_size(unsigned long size, unsigned long step)
{
	RADIX_TREE(tree, GFP_KERNEL);
	long long normal, tagged;

	benchmark_insert(&tree, size, step);
	benchmark_tagging(&tree, size, step);

	tagged = benchmark_iter(&tree, true);
	normal = benchmark_iter(&tree, false);

	printv(2, "Size: %8ld, step: %8ld, tagged iteration: %8lld ns\n",
		size, step, tagged);
	printv(2, "Size: %8ld, step: %8ld, normal iteration: %8lld ns\n",
		size, step, normal);

	benchmark_delete(&tree, size, step);

	item_kill_tree(&tree);
	rcu_barrier();
}

void benchmark(void)
{
	unsigned long size[] = {1 << 10, 1 << 20, 0};
	unsigned long step[] = {1, 2, 7, 15, 63, 64, 65,
				128, 256, 512, 12345, 0};
	int c, s;

	printv(1, "starting benchmarks\n");
	printv(1, "RADIX_TREE_MAP_SHIFT = %d\n", RADIX_TREE_MAP_SHIFT);

	for (c = 0; size[c]; c++)
		for (s = 0; step[s]; s++)
			benchmark_size(size[c], step[s]);
}

static long benchmark_xas_size(struct xarray *xa,
        unsigned long size, unsigned long skip)
{
        unsigned long i, store=0, erase=0;

        for (i = 0; i < size; i++) {
		xa_store(xa, i, xa_mk_value(i & LONG_MAX), GFP_KERNEL);
		store++;
        }
        for (i = 0; i < size; i++) {
                if (!(i % skip)) continue;
                xa_erase(xa, i);
                erase++;
        }
        return store - erase;
}

void benchmark_xas_perf(void)
{
        static DEFINE_XARRAY(xarray);
        static XA_STATE(xas, &xarray, 0);
	void *entry;
	unsigned long index, value;
	unsigned long seen=0;

	struct timespec start, finish;
        long store, nsec1, nsec2;
        const unsigned long size = 1 << 20;
        const unsigned long skip = 7;

        printv(1, "starting benchmark_xas_perf\n");

        store = benchmark_xas_size(&xarray, size, skip);

        clock_gettime(CLOCK_MONOTONIC, &start);
        rcu_read_lock();
        xas_for_each(&xas, entry, ULONG_MAX) {
                value = xa_to_value(entry);
                index = value % skip;
                WARN_ON(index);
		seen++;
	}
        rcu_read_unlock();
        clock_gettime(CLOCK_MONOTONIC, &finish);
        nsec1 = (finish.tv_sec - start.tv_sec) * NSEC_PER_SEC +
	       (finish.tv_nsec - start.tv_nsec);
        printv(2, "store=%ld, seen=%lu, elapsed=%lu(ns)\n",
                        store, seen, nsec1);
        WARN_ON(store != seen);

        xa_destroy(&xarray);
        store = benchmark_xas_size(&xarray, size, skip);

        seen = 0;
        xas_set(&xas, 0);
        clock_gettime(CLOCK_MONOTONIC, &start);
        rcu_read_lock();
        xas_for_each_next_fast(&xas, entry, ULONG_MAX) {
                value = xa_to_value(entry);
                index = value % skip;
                WARN_ON(index);
                seen++;
        }
        rcu_read_unlock();
        clock_gettime(CLOCK_MONOTONIC, &finish);
        nsec2 = (finish.tv_sec - start.tv_sec) * NSEC_PER_SEC +
	       (finish.tv_nsec - start.tv_nsec);
        printv(2, "store=%ld, seen=%lu, elapsed=%lu(ns)\n",
                        store, seen, nsec2);
        WARN_ON(store != seen);

        xa_destroy(&xarray);

        printv(2, "perf=%f(%%)\n",
                ((double)(nsec2-nsec1) / (double)(nsec2+nsec1)) * 100);
}

