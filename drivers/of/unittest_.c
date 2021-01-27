// SPDX-License-Identifier: GPL-2.0
/*
 * Self tests for device tree subsystem
 */

#define pr_fmt(fmt) "### dt-test ### " fmt

#include "test/dtb-test.h"
#include "test/debug.h"

#include <linux/memblock.h>
//#include <linux/clk.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/hashtable.h>
#include <linux/libfdt.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include <linux/i2c.h>
//#include <linux/i2c-mux.h>

#include <linux/bitops.h>

#include "of_private.h"

static struct unittest_results {
        int passed;
        int failed;
} unittest_results;

#define unittest(result, fmt, ...) ({ \
        bool failed = !(result); \
        if (failed) { \
                unittest_results.failed++; \
                pr_err("FAIL %s(): lines: %i " fmt, __func__, __LINE__, ##__VA_ARGS__); \
        } else { \
                unittest_results.passed++; \
                pr_debug("PASS %s(): lines: %i\n", __func__, __LINE__); \
        } \
        failed; \
})
//47
static void __init of_unittest_find_node_by_name(void)
{
    pr_fn_start_on(stack_depth);

        struct device_node *np;
        const char *options;	//, *name;

        np = of_find_node_by_path("/testcase-data");
        //name = kasprintf(GFP_KERNEL, "%pOF", np);
        pr_info_view_on(stack_depth, "%20s : %s\n", np->name);
        unittest(np && !strcmp("testcase-data", np->name),
                "find /testcase-data failed\n");
        of_node_put(np);
        //kfree(name);

        /* Test if trailing '/' works */
        np = of_find_node_by_path("/testcase-data/");
        unittest(!np, "trailing '/' on /testcase-data/ should fail\n");

        np = of_find_node_by_path("/testcase-data/phandle-tests/consumer-a");
        //name = kasprintf(GFP_KERNEL, "%pOF", np);
        pr_info_view_on(stack_depth, "%20s : %s\n", np->name);
        unittest(np && !strcmp("consumer-a", np->name),
                "find /testcase-data/phandle-tests/consumer-a failed\n");
        of_node_put(np);
        //kfree(name);

        np = of_find_node_by_path("testcase-alias");
        //name = kasprintf(GFP_KERNEL, "%pOF", np);
        pr_info_view_on(stack_depth, "%20s : %s\n", np->name);
        unittest(np && !strcmp("testcase-data", np->name),
                "find testcase-alias failed\n");
        of_node_put(np);
        //kfree(name);

        /* Test if trailing '/' works on aliases */
        np = of_find_node_by_path("testcase-alias/");
        unittest(!np, "trailing '/' on testcase-alias/ should fail\n");

        np = of_find_node_by_path("testcase-alias/phandle-tests/consumer-a");
        //name = kasprintf(GFP_KERNEL, "%pOF", np);
        pr_info_view_on(stack_depth, "%20s : %s\n", np->name);
        unittest(np && !strcmp("consumer-a", np->name),
                "find testcase-alias/phandle-tests/consumer-a failed\n");
        of_node_put(np);
        //kfree(name);

        np = of_find_node_by_path("/testcase-data/missing-path");
        unittest(!np, "non-existent path returned node %pOF\n", np);
        of_node_put(np);

        np = of_find_node_opts_by_path("/testcase-data:testoption", &options);
        pr_info_view_on(stack_depth, "%20s : %s\n", np->name);
        pr_info_view_on(stack_depth, "%20s : %s\n",options);
        unittest(np && !strcmp("testoption", options),
                 "option path test failed\n");
        of_node_put(np);

        np = of_find_node_opts_by_path("/testcase-data:test/option", &options);
        unittest(np && !strcmp("test/option", options),
                 "option path test, subcase #1 failed\n");
        of_node_put(np);

        np = of_find_node_opts_by_path("/testcase-data/testcase-device1:test/option", &options);
        unittest(np && !strcmp("test/option", options),
                 "option path test, subcase #2 failed\n");
        of_node_put(np);

        np = of_find_node_opts_by_path("/testcase-data:testoption", NULL);
        unittest(np, "NULL option path test failed\n");
        of_node_put(np);

        np = of_find_node_opts_by_path("testcase-alias:testaliasoption",
                                       &options);
        unittest(np && !strcmp("testaliasoption", options),
                 "option alias path test failed\n");
        of_node_put(np);

        np = of_find_node_opts_by_path("testcase-alias:test/alias/option",
                                       &options);
        pr_info_view_on(stack_depth, "%20s : %s\n", np->name);
        pr_info_view_on(stack_depth, "%20s : %s\n",options);
        unittest(np && !strcmp("test/alias/option", options),
                 "option alias path test, subcase #1 failed\n");
        of_node_put(np);

        np = of_find_node_opts_by_path("testcase-alias:testaliasoption", NULL);
        unittest(np, "NULL option alias path test failed\n");
        of_node_put(np);

        options = "testoption";
        np = of_find_node_opts_by_path("testcase-alias", &options);
        unittest(np && !options, "option clearing test failed\n");
        of_node_put(np);

        options = "testoption";
        np = of_find_node_opts_by_path("/", &options);
        unittest(np && !options, "option clearing root node test failed\n");
        of_node_put(np);

    pr_fn_end_on(stack_depth);
}
//146
static void __init of_unittest_dynamic(void)
{
    pr_fn_start_on(stack_depth);

        struct device_node *np;
        struct property *prop;

        np = of_find_node_by_path("/testcase-data");
        if (!np) {
                pr_err("missing testcase data\n");
                return;
        }

        /* Array of 4 properties for the purpose of testing */
        prop = kcalloc(4, sizeof(*prop), GFP_KERNEL);
        if (!prop) {
                unittest(0, "kzalloc() failed\n");
                return;
        }

        /* Add a new property - should pass*/
        prop->name = "new-property";
        prop->value = "new-property-data";
        prop->length = strlen(prop->value) + 1;
        unittest(of_add_property(np, prop) == 0, "Adding a new property failed\n");

        /* Try to add an existing property - should fail */
        prop++;
        prop->name = "new-property";
        prop->value = "new-property-data-should-fail";
        prop->length = strlen(prop->value) + 1;
        unittest(of_add_property(np, prop) != 0,
                 "Adding an existing property should have failed\n");

        /* Try to modify an existing property - should pass */
        prop->value = "modify-property-data-should-pass";
        prop->length = strlen(prop->value) + 1;
        unittest(of_update_property(np, prop) == 0,
                 "Updating an existing property should have passed\n");

        /* Try to modify non-existent property - should pass*/
        prop++;
        prop->name = "modify-property";
        prop->value = "modify-missing-property-data-should-pass";
        prop->length = strlen(prop->value) + 1;
        unittest(of_update_property(np, prop) == 0,
             "Updating a missing property should have passed\n");

        /* Remove property - should pass */
        unittest(of_remove_property(np, prop) == 0,
             "Removing a property should have passed\n");

        /* Adding very large property - should pass */
        prop++;
        prop->name = "large-property-PAGE_SIZEx8";
        prop->length = PAGE_SIZE * 8;
        prop->value = kzalloc(prop->length, GFP_KERNEL);
        unittest(prop->value != NULL, "Unable to allocate large buffer\n");
        if (prop->value)
            unittest(of_add_property(np, prop) == 0,
                 "Adding a large property should have passed\n");

    pr_fn_end_on(stack_depth);
}
//207
static int __init of_unittest_check_node_linkage(struct device_node *np)
{
        struct device_node *child;
        int count = 0, rc;

        for_each_child_of_node(np, child) {
                if (child->parent != np) {
                        pr_err("Child node %pOFn links to wrong parent %pOFn\n",
                                 child, np);
                        rc = -EINVAL;
                        goto put_child;
                }

                rc = of_unittest_check_node_linkage(child);
                if (rc < 0)
                        goto put_child;
                count += rc;
        }

        return count + 1;
put_child:
        of_node_put(child);
        return rc;
}
//232
static void __init of_unittest_check_tree_linkage(void)
{
    pr_fn_start_on(stack_depth);

        struct device_node *np;
        int allnode_count = 0, child_count;

        if (!of_root)
                return;

        for_each_of_allnodes(np)
                allnode_count++;
        child_count = of_unittest_check_node_linkage(of_root);

        pr_info_view_on(stack_depth, "%20s : %d\n", allnode_count);
        pr_info_view_on(stack_depth, "%20s : %d\n", child_count);

        unittest(child_count > 0, "Device node data structure is corrupted\n");
        unittest(child_count == allnode_count,
                 "allnodes list size (%i) doesn't match sibling lists size (%i)\n",
                 allnode_count, child_count);
        pr_debug("allnodes list size (%i); sibling lists size (%i)\n", allnode_count, child_count);

    pr_fn_end_on(stack_depth);
}
//251 lines



//319 lines
struct node_hash {
        struct hlist_node node;
        struct device_node *np;
};

static DEFINE_HASHTABLE(phandle_ht, 8);
static void __init of_unittest_check_phandles(void)
{
    pr_fn_start_on(stack_depth);

        struct device_node *np;
        struct node_hash *nh;
        struct hlist_node *tmp;
        int i, dup_count = 0, phandle_count = 0;

        for_each_of_allnodes(np) {
                if (!np->phandle)
                        continue;

                hash_for_each_possible(phandle_ht, nh, node, np->phandle) {
                        if (nh->np->phandle == np->phandle) {
                                //pr_info("Duplicate phandle! %i used by %pOF and %pOF\n",
                                //        np->phandle, nh->np, np);
                                dup_count++;
                                break;
                        }
                }

                nh = kzalloc(sizeof(*nh), GFP_KERNEL);
                if (!nh)
                        return;

                nh->np = np;
                hash_add(phandle_ht, &nh->node, np->phandle);
                phandle_count++;
        }

        pr_info_view_on(stack_depth, "%20s : %d\n", phandle_count);
        pr_info_view_on(stack_depth, "%20s : %d\n", dup_count);

        unittest(dup_count == 0, "Found %i duplicates in %i phandles\n",
                 dup_count, phandle_count);

        /* Clean up */
        hash_for_each_safe(phandle_ht, i, tmp, nh, node) {
                hash_del(&nh->node);
                //kfree(nh);	//error!
        }

    pr_fn_end_on(stack_depth);
}
//363 lines




//1081 lines
/**
 *      update_node_properties - adds the properties
 *      of np into dup node (present in live tree) and
 *      updates parent of children of np to dup.
 *
 *      @np:    node whose properties are being added to the live tree
 *      @dup:   node present in live tree to be updated
 */
static void update_node_properties(struct device_node *np,
                                        struct device_node *dup)
{
        struct property *prop;
        struct property *save_next;
        struct device_node *child;
        int ret;

        for_each_child_of_node(np, child)
                child->parent = dup;

        /*
         * "unittest internal error: unable to add testdata property"
         *
         *    If this message reports a property in node '/__symbols__' then
         *    the respective unittest overlay contains a label that has the
         *    same name as a label in the live devicetree.  The label will
         *    be in the live devicetree only if the devicetree source was
         *    compiled with the '-@' option.  If you encounter this error,
         *    please consider renaming __all__ of the labels in the unittest
         *    overlay dts files with an odd prefix that is unlikely to be
         *    used in a real devicetree.
         */

        /*
         * open code for_each_property_of_node() because of_add_property()
         * sets prop->next to NULL
         */
        for (prop = np->properties; prop != NULL; prop = save_next) {
                save_next = prop->next;
                ret = of_add_property(dup, prop);
                if (ret) {
                        if (ret == -EEXIST && !strcmp(prop->name, "name"))
                                continue;
                        pr_err("unittest internal error: unable to add testdata property %pOF/%s",
                               np, prop->name);
                }
        }
}
//1129
/**
 *      attach_node_and_children - attaches nodes
 *      and its children to live tree.
 *      CAUTION: misleading function name - if node @np already exists in
 *      the live tree then children of @np are *not* attached to the live
 *      tree.  This works for the current test devicetree nodes because such
 *      nodes do not have child nodes.
 *
 *      @np:    Node to attach to live tree
 */
static void attach_node_and_children(struct device_node *np)
{
    pr_fn_start_on(stack_depth);

        struct device_node *next, *dup, *child;
        unsigned long flags;
        const char *full_name;

        full_name = kasprintf(GFP_KERNEL, "%pOF", np);

        pr_info_view_on(stack_depth, "%20s : %s\n", full_name);

        if (!strcmp(full_name, "/__local_fixups__") ||
            !strcmp(full_name, "/__fixups__"))
                return;

        dup = of_find_node_by_path(full_name);
        kfree(full_name);
        if (dup) {
                update_node_properties(np, dup);
                return;
        }

        child = np->child;
        np->child = NULL;

        mutex_lock(&of_mutex);
        raw_spin_lock_irqsave(&devtree_lock, flags);
        np->sibling = np->parent->child;
        np->parent->child = np;
        of_node_clear_flag(np, OF_DETACHED);
        raw_spin_unlock_irqrestore(&devtree_lock, flags);

        __of_attach_node_sysfs(np);
        mutex_unlock(&of_mutex);

        while (child) {
                next = child->sibling;
                attach_node_and_children(child);
                child = next;
        }

    pr_fn_end_on(stack_depth);
}
//1178 lines




//1178 lines
/**
 *      unittest_data_add - Reads, copies data from
 *      linked tree and attaches it to the live tree
 */
static int __init unittest_data_add(void)
{
    pr_fn_start_on(stack_depth);

        void *unittest_data;
        struct device_node *unittest_data_node, *np;
        /*
         * __dtb_testcases_begin[] and __dtb_testcases_end[] are magically
         * created by cmd_dt_S_dtb in scripts/Makefile.lib
         */
        int rc;

        /* creating copy */
        //unittest_data = kmemdup(__dtb_testcases_begin, size, GFP_KERNEL);
        //unittest_data = kmemdup(dtb_early_va, dtb_size, GFP_KERNEL);
        unittest_data = dtb_early_va;
        if (!unittest_data)
                return -ENOMEM;

        //lib/hex_dump.c
        /*
        print_hex_dump(KERN_DEBUG, "raw data: ", DUMP_PREFIX_OFFSET,
                16, 1, unittest_data, dtb_size, true);
        */

        of_fdt_unflatten_tree(unittest_data, NULL, &unittest_data_node);
        if (!unittest_data_node) {
                pr_warn("%s: No tree to attach; not running tests\n", __func__);
                kfree(unittest_data);
                return -ENODATA;
        }

        /*
         * This lock normally encloses of_resolve_phandles()
         */
        of_overlay_mutex_lock();

        rc = of_resolve_phandles(unittest_data_node);
        if (rc) {
                pr_err("%s: Failed to resolve phandles (rc=%i)\n", __func__, rc);
                of_overlay_mutex_unlock();
                return -EINVAL;
        }

        pr_info_view_on(stack_depth, "%20s : %p\n", of_root);
        if (!of_root) {
            of_root = unittest_data_node;
            pr_info_view_on(stack_depth, "%20s : %p\n", of_root);
            for_each_of_allnodes(np)
                __of_attach_node_sysfs(np);
            of_aliases = of_find_node_by_path("/aliases");
            pr_info_view_on(stack_depth, "%20s : %p\n", of_aliases);
            of_chosen = of_find_node_by_path("/chosen");
            pr_info_view_on(stack_depth, "%20s : %p\n", of_chosen);
            of_overlay_mutex_unlock();
            return 0;
        }

        /* attach the sub-tree to live tree */
        np = unittest_data_node->child;
        while (np) {
            struct device_node *next = np->sibling;

            pr_info_view_on(stack_depth, "%20s : %s\n", np->name);

            np->parent = of_root;
            attach_node_and_children(np);
            np = next;
        }

        of_overlay_mutex_unlock();

    pr_fn_end_on(stack_depth);
    return 0;
}
//1250 lines



//2521 lines
void dtb_of_unittest(void)
{
    pr_fn_start_on(stack_depth);

    struct device_node *np;
    int res;

    /* adding data for unittest */

    res = unittest_data_add();
    if (res)
        return;

    if (!of_aliases)
        of_aliases = of_find_node_by_path("/aliases");

    np = of_find_node_by_path("/testcase-data/phandle-tests/consumer-a");
    if (!np) {
        pr_info("No testcase data in device tree; not running tests\n");
        return 0;
    }
    of_node_put(np);

    pr_info("start of unittest - you will see error messages\n");
    of_unittest_check_tree_linkage();
    //of_unittest_check_phandles();	//error!
    of_unittest_find_node_by_name();
    of_unittest_dynamic();

#if 0
    of_unittest_parse_phandle_with_args();
    of_unittest_parse_phandle_with_args_map();
    of_unittest_printf();
    of_unittest_property_string();
    of_unittest_property_copy();
    of_unittest_changeset();
    of_unittest_parse_interrupts();
    of_unittest_parse_interrupts_extended();
    of_unittest_match_node();
    of_unittest_platform_populate();
    of_unittest_overlay();

    /* Double check linkage after removing testcase data */
    of_unittest_check_tree_linkage();

    of_unittest_overlay_high_level();
#endif

    pr_info("end of unittest - %i passed, %i failed\n",
        unittest_results.passed, unittest_results.failed);

    pr_fn_end_on(stack_depth);
}
