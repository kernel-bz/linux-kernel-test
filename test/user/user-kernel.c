// SPDX-License-Identifier: GPL-2.0
/*
 *  test/user/user-kernel.c
 *  User Functions for kernel
 *
 *  Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stddef.h>

#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/of_irq.h>
#include <linux/irqdomain.h>
#include <linux/kobject.h>


//include/linux/notifier.h
//kernel/nofifier.c
inline int atomic_notifier_chain_register(struct atomic_notifier_head *nh,
        struct notifier_block *nb) { }
inline int blocking_notifier_chain_register(struct blocking_notifier_head *nh,
        struct notifier_block *nb) { }
inline int raw_notifier_chain_register(struct raw_notifier_head *nh,
        struct notifier_block *nb) { }
inline int srcu_notifier_chain_register(struct srcu_notifier_head *nh,
        struct notifier_block *nb) { }

inline int blocking_notifier_chain_cond_register(
        struct blocking_notifier_head *nh,
        struct notifier_block *nb) { }

inline int atomic_notifier_chain_unregister(struct atomic_notifier_head *nh,
        struct notifier_block *nb) { }
inline int blocking_notifier_chain_unregister(struct blocking_notifier_head *nh,
        struct notifier_block *nb) { }
inline int raw_notifier_chain_unregister(struct raw_notifier_head *nh,
        struct notifier_block *nb) { }
inline int srcu_notifier_chain_unregister(struct srcu_notifier_head *nh,
        struct notifier_block *nb) { }

inline int atomic_notifier_call_chain(struct atomic_notifier_head *nh,
        unsigned long val, void *v) { }
inline int __atomic_notifier_call_chain(struct atomic_notifier_head *nh,
    unsigned long val, void *v, int nr_to_call, int *nr_calls) { }
inline int blocking_notifier_call_chain(struct blocking_notifier_head *nh,
        unsigned long val, void *v) { }
inline int __blocking_notifier_call_chain(struct blocking_notifier_head *nh,
    unsigned long val, void *v, int nr_to_call, int *nr_calls) { }
inline int raw_notifier_call_chain(struct raw_notifier_head *nh,
        unsigned long val, void *v) { }
inline int __raw_notifier_call_chain(struct raw_notifier_head *nh,
    unsigned long val, void *v, int nr_to_call, int *nr_calls) { }
inline int srcu_notifier_call_chain(struct srcu_notifier_head *nh,
        unsigned long val, void *v) { }
inline int __srcu_notifier_call_chain(struct srcu_notifier_head *nh,
    unsigned long val, void *v, int nr_to_call, int *nr_calls) { }


//include/linux/of_irq.h
//kernel/irq/irqdomain.c
inline unsigned int irq_create_of_mapping(struct of_phandle_args *irq_data) { }

//include/linux/irqdomain.h
//kernel/irq/irqdomain.c
inline struct irq_domain *irq_find_matching_fwspec(struct irq_fwspec *fwspec,

                        enum irq_domain_bus_token bus_token)
{
    return NULL;
}
EXPORT_SYMBOL_GPL(irq_find_matching_fwspec);


//kernel/ksysfs.c
struct kobject *kernel_kobj;
EXPORT_SYMBOL_GPL(kernel_kobj);
