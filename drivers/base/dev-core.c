
#include "test/debug.h"
#include "test/define-usr-dev.h"

#include <linux/device.h>
#include <linux/export.h>



//1126 lines
static struct kobj_type device_ktype = {
//.release	= device_release,
//.sysfs_ops	= &dev_sysfs_ops,
//.namespace	= device_namespace,
//.get_ownership	= device_get_ownership,
};



//1529 lines
/* /sys/devices/ */
struct kset *devices_kset;



//1696 lines
void device_initialize(struct device *dev)
{
    dev->kobj.kset = devices_kset;
    //kobject_init(&dev->kobj, &device_ktype);
    INIT_LIST_HEAD(&dev->dma_pools);
    mutex_init(&dev->mutex);
#ifdef CONFIG_PROVE_LOCKING
    mutex_init(&dev->lockdep_mutex);
#endif
    //lockdep_set_novalidate_class(&dev->mutex);
    spin_lock_init(&dev->devres_lock);
    INIT_LIST_HEAD(&dev->devres_head);
    device_pm_init(dev);
    set_dev_node(dev, -1);
#ifdef CONFIG_GENERIC_MSI_IRQ
    INIT_LIST_HEAD(&dev->msi_list);
#endif
    INIT_LIST_HEAD(&dev->links.consumers);
    INIT_LIST_HEAD(&dev->links.suppliers);
    dev->links.status = DL_DEV_NO_DRIVER;
}
EXPORT_SYMBOL_GPL(device_initialize);
//1719 lines



//2098 lines
int device_add(struct device *dev)
{
    return 0;
}
EXPORT_SYMBOL_GPL(device_add);



//2286 lines
struct device *get_device(struct device *dev)
{
    return NULL;
}
EXPORT_SYMBOL_GPL(get_device);

void put_device(struct device *dev)
{
    /* might_sleep(); */
    if (dev)
        kobject_put(&dev->kobj);
}
EXPORT_SYMBOL_GPL(put_device);
//2304 lines


