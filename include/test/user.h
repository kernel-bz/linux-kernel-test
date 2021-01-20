#ifndef __TEST_USER_H
#define __TEST_USER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "test/define-usr.h"

//include/linux/etherdevice.h
static inline bool is_valid_ether_addr(const u8 *addr)
{
    /* FF:FF:FF:FF:FF:FF is a multicast address so we don't need to
     * explicitly check for it here. */
    //return !is_multicast_ether_addr(addr) && !is_zero_ether_addr(addr);
    return true;
}

//include/linux/console.h
//kernel/printk/printk.c
static int add_preferred_console(char *name, int idx, char *options) {}

#ifdef __cplusplus
}
#endif

#endif // __TEST_BASIC_H
