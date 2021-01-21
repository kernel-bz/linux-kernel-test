#ifndef __TEST_USER_H
#define __TEST_USER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "test/user-define.h"


//include/linux/console.h
//kernel/printk/printk.c
static int add_preferred_console(char *name, int idx, char *options) {}


//include/linux/irq.h
//kernel/irq/chip.c
static inline struct irq_data *irq_get_irq_data(unsigned int irq)
{
    //struct irq_desc *desc = irq_to_desc(irq);
    //return desc ? &desc->irq_data : NULL;
    return NULL;
}

#ifdef __cplusplus
}
#endif

#endif // __TEST_BASIC_H
