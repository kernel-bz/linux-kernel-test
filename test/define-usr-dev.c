/* SPDX-License-Identifier: GPL-2.0 */
/*
 *	test/define-usr-dev.c
 *
 * 	Copyright (C) 2020, www.kernel.bz
 * 	Author: JaeJoon Jung <rgbi3307@gmail.com>
 *
 *	user defined functions for devices
 */

#include <stddef.h>

#include <linux/device.h>
#include <linux/export.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/kernel.h>

#include <linux/notifier.h>
#include <linux/irqdomain.h>
#include <linux/phy.h>


//drivers/base/devres.c: 121 lines
#ifdef CONFIG_DEBUG_DEVRES
void * __devres_alloc_node(dr_release_t release, size_t size, gfp_t gfp, int nid,
              const char *name)
{
    struct devres *dr;

    dr = alloc_dr(release, size, gfp | __GFP_ZERO, nid);
    if (unlikely(!dr))
        return NULL;
    set_node_dbginfo(&dr->node, name, size);
    return dr->data;
}
EXPORT_SYMBOL_GPL(__devres_alloc_node);
#else
void * devres_alloc_node(dr_release_t release, size_t size, gfp_t gfp, int nid)
{
    return NULL;
}
EXPORT_SYMBOL_GPL(devres_alloc_node);
#endif


//drivers/base/devres.c: 209 lines
void devres_free(void *res)
{
}
EXPORT_SYMBOL_GPL(devres_free);


//drivers/base/devres.c: 229 lines
void devres_add(struct device *dev, void *res) { }


//drivers/base/devres.c: 376 lines
int devres_destroy(struct device *dev, dr_release_t release,
           dr_match_t match, void *match_data)
{
    return 0;
}
EXPORT_SYMBOL_GPL(devres_destroy);


//drivers/base/devres.c: 406 lines
int devres_release(struct device *dev, dr_release_t release,
           dr_match_t match, void *match_data)
{
    return 0;
}
EXPORT_SYMBOL_GPL(devres_release);



//kernel/printk/printk.c: 2215 lines
int add_preferred_console(char *name, int idx, char *options)
{
    //return __add_preferred_console(name, idx, options, NULL);
    return 0;
}

//drivers/base/core.c: 2405 lines
void device_unregister(struct device *dev)
{
    //pr_debug("device: '%s': %s\n", dev_name(dev), __func__);
    //device_del(dev);
    //put_device(dev);
}
EXPORT_SYMBOL_GPL(device_unregister);



//kernel/notifier.c: 215 lines
int blocking_notifier_chain_register(struct blocking_notifier_head *nh,
        struct notifier_block *n)
{
    return 0;
}
EXPORT_SYMBOL_GPL(blocking_notifier_chain_register);


//kernel/notifier.c: 268 lines
int blocking_notifier_chain_unregister(struct blocking_notifier_head *nh,
        struct notifier_block *n)
{
    return 0;
}
EXPORT_SYMBOL_GPL(blocking_notifier_chain_unregister);


//kernel/notifier.c: 327 lines
int blocking_notifier_call_chain(struct blocking_notifier_head *nh,
        unsigned long val, void *v)
{
    //return __blocking_notifier_call_chain(nh, val, v, -1, NULL);
    return 0;
}
EXPORT_SYMBOL_GPL(blocking_notifier_call_chain);



//kernel/irq/irqdomain.c: 852 lines
unsigned int irq_create_of_mapping(struct of_phandle_args *irq_data)
{
    return 0;
}
EXPORT_SYMBOL_GPL(irq_create_of_mapping);

//kernel/irq/irqdomain.c: 384 lines
struct irq_domain *irq_find_matching_fwspec(struct irq_fwspec *fwspec,
                        enum irq_domain_bus_token bus_token)
{
    return NULL;
}
EXPORT_SYMBOL_GPL(irq_find_matching_fwspec);


//kernel/irq/chip.c: 158 lines
struct irq_data *irq_get_irq_data(unsigned int irq)
{
    return NULL;
}
EXPORT_SYMBOL_GPL(irq_get_irq_data);


//drivers/base/bus.c: 325 lines
struct device *bus_find_device(struct bus_type *bus,
                   struct device *start, const void *data,
                   int (*match)(struct device *dev, const void *data))
{
    return NULL;
}
EXPORT_SYMBOL_GPL(bus_find_device);



//drivers/base/core.c: 3440 lines
/**
 * device_set_of_node_from_dev - reuse device-tree node of another device
 * @dev: device whose device-tree node is being set
 * @dev2: device whose device-tree node is being reused
 *
 * Takes another reference to the new device-tree node after first dropping
 * any reference held to the old node.
 */
void device_set_of_node_from_dev(struct device *dev, const struct device *dev2)
{
    //of_node_put(dev->of_node);
    //dev->of_node = of_node_get(dev2->of_node);
    //dev->of_node_reused = true;
}
EXPORT_SYMBOL_GPL(device_set_of_node_from_dev);

int device_match_name(struct device *dev, const void *name)
{
    //return sysfs_streq(dev_name(dev), name);
    return 0;
}
EXPORT_SYMBOL_GPL(device_match_name);

int device_match_of_node(struct device *dev, const void *np)
{
    return dev->of_node == np;
}
EXPORT_SYMBOL_GPL(device_match_of_node);

int device_match_fwnode(struct device *dev, const void *fwnode)
{
    //return dev_fwnode(dev) == fwnode;
    return 0;
}
EXPORT_SYMBOL_GPL(device_match_fwnode);

int device_match_devt(struct device *dev, const void *pdevt)
{
    return dev->devt == *(dev_t *)pdevt;
}
EXPORT_SYMBOL_GPL(device_match_devt);

int device_match_acpi_dev(struct device *dev, const void *adev)
{
    //return ACPI_COMPANION(dev) == adev;
    return 0;
}
EXPORT_SYMBOL(device_match_acpi_dev);

int device_match_any(struct device *dev, const void *unused)
{
    return 1;
}
EXPORT_SYMBOL_GPL(device_match_any);



//drivers/net/phy/mdio_bus.c: 726 lines
struct bus_type mdio_bus_type = {
    .name		= "mdio_bus",
    //.match		= mdio_bus_match,
    //.uevent		= mdio_uevent,
};
EXPORT_SYMBOL(mdio_bus_type);



//drivers/net/phy/phy_device.c: 945 lines
int phy_connect_direct(struct net_device *dev, struct phy_device *phydev,
               void (*handler)(struct net_device *),
               phy_interface_t interface)
{
    return 0;
}
EXPORT_SYMBOL(phy_connect_direct);


//drivers/net/phy/phy_device.c: 1192 lines
int phy_attach_direct(struct net_device *dev, struct phy_device *phydev,
              u32 flags, phy_interface_t interface)
{
    return 0;
}
EXPORT_SYMBOL(phy_attach_direct);



//drivers/net/phy/mdio_device.c: 25 lines
void mdio_device_free(struct mdio_device *mdiodev)
{
    //put_device(&mdiodev->dev);
}
EXPORT_SYMBOL(mdio_device_free);

//drivers/net/phy/mdio_device.c: 47 lines
struct mdio_device *mdio_device_create(struct mii_bus *bus, int addr)
{
    return NULL;
}
EXPORT_SYMBOL(mdio_device_create);

//drivers/net/phy/mdio_device.c: 76 lines
int mdio_device_register(struct mdio_device *mdiodev)
{
    return 0;
}
EXPORT_SYMBOL(mdio_device_register);


//drivers/net/phy/phy_device.c: 575 lines
struct phy_device *phy_device_create(struct mii_bus *bus, int addr, int phy_id,
                     bool is_c45,
                     struct phy_c45_device_ids *c45_ids)
{
    return NULL;
}
EXPORT_SYMBOL(phy_device_create);



//arch
void * __init prom_early_alloc(unsigned long size)
{
    return NULL;
}


//drivers/base/core.c: 2000 lines
int dev_set_name(struct device *dev, const char *fmt, ...)
{
    return 0;
}
EXPORT_SYMBOL_GPL(dev_set_name);


//drivers/base/core.c: 2496 lines
int device_for_each_child(struct device *parent, void *data,
              int (*fn)(struct device *dev, void *data))
{
    return 0;
}
EXPORT_SYMBOL_GPL(device_for_each_child);


//drivers/net/phy/mdio_bus.c: 367 lines
int __mdiobus_register(struct mii_bus *bus, struct module *owner)
{
    return 0;
}
EXPORT_SYMBOL(__mdiobus_register);


//drivers/net/phy/mdio_bus.c: 449 lines
void mdiobus_unregister(struct mii_bus *bus)
{
}
EXPORT_SYMBOL(mdiobus_unregister);


//net/ethernet/eth.c: 573 lines
int nvmem_get_mac_address(struct device *dev, void *addrbuf)
{
    return 0;
}
EXPORT_SYMBOL(nvmem_get_mac_address);


//drivers/base/devres.c: 966 lines
void *devm_kmemdup(struct device *dev, const void *src, size_t len, gfp_t gfp)
{
    void *p;

    //p = devm_kmalloc(dev, len, gfp);
    p = kmalloc(len, gfp);
    if (p)
        memcpy(p, src, len);

    return p;
}
EXPORT_SYMBOL_GPL(devm_kmemdup);


//drivers/net/phy/mdio_bus.c: 130 lines
bool mdiobus_is_registered_device(struct mii_bus *bus, int addr)
{
    return bus->mdio_map[addr];
}
EXPORT_SYMBOL(mdiobus_is_registered_device);


//mm/slab.c: 3662 lines
void *__kmalloc(size_t size, gfp_t flags)
{
    //return __do_kmalloc(size, flags, _RET_IP_);
    return kmalloc(size, flags);
}
EXPORT_SYMBOL(__kmalloc);

void *__kmalloc_track_caller(size_t size, gfp_t flags, unsigned long caller)
{
    //return __do_kmalloc(size, flags, caller);
    return kmalloc(size, flags);
}
EXPORT_SYMBOL(__kmalloc_track_caller);
