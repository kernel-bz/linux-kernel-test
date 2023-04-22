// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/algorithm/hlist01.c
 *  Hash List Test 01
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stdio.h>
#include <linux/stddef.h>
#include <linux/list.h>
#include <linux/hashtable.h>
#include <linux/idr.h>
#include <linux/jhash.h>

#include "test/debug.h"
#include "test/user-types.h"

#include "lib/list-test.c"

//include/linux/types.h
/**
struct hlist_head {
        struct hlist_node *first;
};

struct hlist_node {
        struct hlist_node *next, **pprev;
};
*/

struct _worker_pool {
    int cpu;
    int	node;
    int id;
    int refcnt;
    struct ida 	worker_ida;
    struct hlist_node	hash_node;
};

//struct hlist_head name[64]
static DEFINE_HASHTABLE(unbound_pool_hash, 6); 	//[64] == 2^6

static u32 _wqattrs_hash(const struct workqueue_attrs *attrs)
{
    u32 hash = 0;

    hash = jhash_1word(attrs->nice, hash);
    hash = jhash(cpumask_bits(attrs->cpumask),
             BITS_TO_LONGS(nr_cpumask_bits) * sizeof(long), hash);
    return hash;
}

static struct _worker_pool *_alloc_init_pool(int val)
{
    struct _worker_pool *pool;
    pool = kzalloc_node(sizeof(struct _worker_pool), GFP_KERNEL, -1);
    if (!pool)
        return;
    pool->cpu = val;
    pool->id = val;
    pool->refcnt = val;

    return pool;
}

void hlist_test01(void)
{
    pr_fn_start_on(stack_depth);

    //struct hlist_head name[8] == 1 << 3 == 2^3
    static DEFINE_HASHTABLE(hash_table, 3);
    u32 i, key = 0, index;
    u32 count[ARRAY_SIZE(hash_table)] = {0, };

    for (i = 0; i < 800; i++) {
        key = i;
        index = hash_min(key, HASH_BITS(hash_table));
        count[index]++;
    }

    for (i = 0; i < ARRAY_SIZE(hash_table); i++)
        pr_out_on(stack_depth, "i=%u, count=%u\n", i, count[i]);

    for (i = 0; i < ARRAY_SIZE(hash_table); i++)
        count[i] = 0;

    pr_out_on(stack_depth, "\n");
    for (i = 0; i < 800; i++) {
        key = jhash_1word(key, i);
        index = hash_min(key, HASH_BITS(hash_table));
        count[index]++;
    }
    for (i = 0; i < ARRAY_SIZE(hash_table); i++)
        pr_out_on(stack_depth, "i=%u, count=%u\n", i, count[i]);

    pr_fn_end_on(stack_depth);
}

/**
 * @brief hlist_test02
 * struct hlist_test_struct {
 * 	  	int data;
 *		struct hlist_node list;
 *	};
 */
void hlist_test02(void)
{
    //refer: hlist_test_for_each_entry() in the lib/list-test.c
    struct hlist_test_struct entries[5], *cur;
    HLIST_HEAD(list);
    int i = 0;

    entries[0].data = 0;
    hlist_add_head(&entries[0].list, &list);
    for (i = 1; i < 5; ++i) {
        entries[i].data = i;
        hlist_add_behind(&entries[i].list, &entries[i-1].list);
    }

    i = 0;

    //cur == pos
    //(pos, head, member)
    hlist_for_each_entry(cur, &list, list) {
        //KUNIT_EXPECT_EQ(test, cur->data, i);
        pr_out_on(stack_depth, "i=%u, data=%d\n", i, cur->data);
        i++;
    }
}

void hlist_test03(void)
{
    struct workqueue_attrs *attrs = alloc_workqueue_attrs();
    struct _worker_pool *pool;
    int target_node = NUMA_NO_NODE;	//-1
    u32 hash;

    pr_fn_start_on(stack_depth);

    attrs->nice = 10;
    hash = _wqattrs_hash(attrs);

    pool = _alloc_init_pool(0);
    if (!pool)
        return;

    hash_init(unbound_pool_hash);

    pr_view_on(stack_depth, "%20s : %ld\n", sizeof(*pool));
    pr_view_on(stack_depth, "%20s : %u\n", hash);	//3812709799
    pr_view_on(stack_depth, "%30s : %d\n", ARRAY_SIZE(unbound_pool_hash));	//HASH_SIZE
    pr_view_on(stack_depth, "%30s : %d\n", HASH_BITS(unbound_pool_hash));	//ilog2
    pr_view_on(stack_depth, "%40s : %d\n", hash_min(hash, HASH_BITS(unbound_pool_hash)));

    //linux/hashtable.h
    //#define hash_add(hashtable, node, key)
    //	hlist_add_head(node, &hashtable[hash_min(key, HASH_BITS(hashtable))])
    //		unbound_pool_hash[hash_min] = &pool->hash_node
    //			hlist_head->first.. hlist_node->next..
    hash_add(unbound_pool_hash, &pool->hash_node, hash);

    pool = _alloc_init_pool(1);
    if (!pool)
        return;

    //attrs->nice = 11;
    //hash = _wqattrs_hash(attrs);
    hash_add(unbound_pool_hash, &pool->hash_node, hash);
    pr_view_on(stack_depth, "%40s : %d\n", hash_min(hash, HASH_BITS(unbound_pool_hash)));

    //collision
    //hash_add(unbound_pool_hash, &pool->hash_node, hash);

    pool = _alloc_init_pool(2);
    if (!pool)
        return;

    hash_add(unbound_pool_hash, &pool->hash_node, hash);
    pr_view_on(stack_depth, "%40s : %d\n", hash_min(hash, HASH_BITS(unbound_pool_hash)));

    //collision
    //hash_add(unbound_pool_hash, &pool->hash_node, hash);

    //int bkt;
    //hash_for_each(unbound_pool_hash, bkt, pool, hash_node) {
    hash_for_each_possible(unbound_pool_hash, pool, hash_node, hash) {
        pool->refcnt++;
        pr_view_on(stack_depth, "%20s : %d\n", pool->id);
        pr_view_on(stack_depth, "%20s : %d\n", pool->refcnt);
    }

    pr_fn_end_on(stack_depth);

}
