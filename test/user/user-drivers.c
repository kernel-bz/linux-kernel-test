// SPDX-License-Identifier: GPL-2.0
/*
 *  test/user/user-driver.c
 *  User Functions for drivers
 *
 *  Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
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


#if 0

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

#endif


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
