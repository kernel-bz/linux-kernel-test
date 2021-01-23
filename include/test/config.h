#ifndef __TEST_CONFIG_H
#define __TEST_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/kconfig.h>

//Arch Configuraion ----------------------------------
#define ARCH_ALL			0
#define ARCH_X86			1
#define ARCH_X86_64			2
#define ARCH_ARM			3
#define ARCH_ARM64			4
#define ARCH_RISCV			5

#define CONFIG_RUN_ARCH 	ARCH_X86_64

#define CONFIG_X86_TSC

#define CONFIG_ARM64
//#define CONFIG_RISCV


//Data Configuraion ----------------------------------
#define CONFIG_VERSION_1		5			//version
#define CONFIG_VERSION_2		4			//patch
#define CONFIG_VERSION_3		20210123	//test date

#define CONFIG_64BIT
#define CONFIG_64BIT_TIME		1
#define CONFIG_HZ				100

//include/linux/compat.h
#ifndef COMPAT_USE_64BIT_TIME
#define COMPAT_USE_64BIT_TIME 	0
#endif

//xarray.h
#define CONFIG_BASE_SMALL		0
//#define CONFIG_BASE_SMALL		1
//#define CONFIG_XARRAY_MULTI	0
#define CONFIG_XARRAY_MULTI		1

#define CONFIG_CMDLINE_FORCE
#define CONFIG_CMDLINE			"cmd line test"

//CPU Configuration ----------------------------------------
//#define CONFIG_NR_CPUS		4
#define CONFIG_NR_CPUS			8
//#define CONFIG_NR_CPUS		16
#define CONFIG_SMP

#define CONFIG_CPU_FREQ
#define CONFIG_CPUSETS
#define CONFIG_VIRT_CPU_ACCOUNTING_NATIVE
//#define CONFIG_HOTPLUG_CPU
#define CONFIG_INIT_ALL_POSSIBLE
#define CONFIG_CPUMASK_OFFSTACK
#define CONFIG_USE_PERCPU_NUMA_NODE_ID

#define CONFIG_NUMA
#define CONFIG_IRQ_DOMAIN
#define CONFIG_NODES_SHIFT		2

//#define CONFIG_DEBUG_PREEMPT
//#define CONFIG_PROVE_RCU_LIST
#define CONFIG_GENERIC_ARCH_TOPOLOGY

//#define CONFIG_DEBUG_PER_CPU_MAPS

//include/linux/energy_model.h
#define CONFIG_ENERGY_MODEL
#define CONFIG_CPU_FREQ_GOV_SCHEDUTIL

//Schedular Configuraion ----------------------------------
#define CONFIG_THREAD_INFO_IN_TASK

#define CONFIG_CGROUPS
#define CONFIG_CGROUP_CPUACCT

#define CONFIG_CGROUP_SCHED
#define CONFIG_FAIR_GROUP_SCHED
#define CONFIG_CFS_BANDWIDTH
#define CONFIG_RT_GROUP_SCHED

//#define CONFIG_SCHED_SMT
//#define CONFIG_SCHED_MC

//#define CONFIG_SCHED_AUTOGROUP
#define CONFIG_SCHEDSTATS
#define CONFIG_SCHED_INFO
#define CONFIG_SCHED_DEBUG

#define CONFIG_NO_HZ_COMMON
//#define CONFIG_NUMA_BALANCING

#define CONFIG_HAVE_SCHED_AVG_IRQ


//Memory Configuration -----------------------------------
#define CONFIG_ARCH_KEEP_MEMBLOCK
#define CONFIG_HAVE_MEMBLOCK_NODE_MAP


//drivers Configuration -----------------------------------
#define CONFIG_OF
#define CONFIG_OF_ADDRESS
#define CONFIG_OF_DYNAMIC
#define CONFIG_OF_KOBJ
#define CONFIG_OF_OVERLAY
#define CONFIG_OF_IRQ
#define CONFIG_OF_NET
#define CONFIG_OF_MDIO	1
#define CONFIG_OF_FLATTREE
#define CONFIG_OF_RESERVED_MEM
#define CONFIG_OF_NUMA
#define CONFIG_OF_UNITTEST
#define CONFIG_OF_EARLY_FLATTREE


#ifdef __cplusplus
}
#endif

#endif // __CONFIG_H
