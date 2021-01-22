// SPDX-License-Identifier: GPL-2.0
/*
 *  test/user/user-common.c
 *  User Functions for common
 *
 *  Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stddef.h>
#include <stdlib.h>

#include <linux/device.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/atomic.h>

#include <mm/slab.h>

//net/ethernet/eth.c: 573 lines
int nvmem_get_mac_address(struct device *dev, void *addrbuf)
{
    return 0;
}
EXPORT_SYMBOL(nvmem_get_mac_address);


//include/linux/slab.h
//mm/slab.c
void *__kmalloc_track_caller(size_t size, gfp_t flags, unsigned long caller)
{
    //return __do_kmalloc(size, flags, caller);
    return kmalloc(size, flags);
}
EXPORT_SYMBOL(__kmalloc_track_caller);

//mm/slab_common.c
enum slab_state slab_state;

bool slab_is_available(void)
{
    return slab_state >= UP;
}



//lib/vsprintf.c
long simple_strtol(const char *cp, char **endp, unsigned int base)
{
    return strtol(cp, endp, base);
}
EXPORT_SYMBOL(simple_strtol);

unsigned long long simple_strtoull(const char *cp, char **endp, unsigned int base)
{
    return strtoull(cp, endp, base);
}
EXPORT_SYMBOL(simple_strtoull);

unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base)
{
    //return simple_strtoull(cp, endp, base);
    return strtoull(cp, endp, base);
}
EXPORT_SYMBOL(simple_strtoul);
