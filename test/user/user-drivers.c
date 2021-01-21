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

//include/linux/of_pdt.h
//arch/x86/platform/olpc/olpc_dt.c
void * __init prom_early_alloc(unsigned long size)
{
    return NULL;
}

//drivers/base/base.h
//drivers/base/devres.c
int devres_release_all(struct device *dev)
{
    return -ENODEV;
}


