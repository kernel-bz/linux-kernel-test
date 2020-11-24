/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * ethtool.h: Defines for Linux ethtool.
 *
 * Copyright (C) 1998 David S. Miller (davem@redhat.com)
 * Copyright 2001 Jeff Garzik <jgarzik@pobox.com>
 * Portions Copyright 2001 Sun Microsystems (thockin@sun.com)
 * Portions Copyright 2002 Intel (eli.kupermann@intel.com,
 *                                christopher.leech@intel.com,
 *                                scott.feldman@intel.com)
 * Portions Copyright (C) Sun Microsystems 2008
 */

#ifndef _UAPI_LINUX_ETHTOOL_H
#define _UAPI_LINUX_ETHTOOL_H

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/if_ether.h>

#ifndef __KERNEL__
#include <limits.h> /* for INT_MAX */
#endif

#define MAX_NUM_QUEUE		4096

#endif	//_UAPI_LINUX_ETHTOOL_H
