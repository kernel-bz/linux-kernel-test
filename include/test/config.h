#ifndef __TEST_CONFIG_H
#define __TEST_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_64BIT
#define CONFIG_64BIT_TIME		1
#define CONFIG_HZ				100

//include/linux/compat.h
#ifndef COMPAT_USE_64BIT_TIME
#define COMPAT_USE_64BIT_TIME 	0
#endif

#define CONFIG_NR_CPUS			4
//#define CONFIG_NR_CPUS		8
#define CONFIG_SMP

#define CONFIG_THREAD_INFO_IN_TASK

//Schedular Configuraion ----------------------------------
#define CONFIG_CGROUPS
#define CONFIG_CGROUP_CPUACCT

#define CONFIG_CGROUP_SCHED
#define CONFIG_FAIR_GROUP_SCHED
#define CONFIG_CFS_BANDWIDTH
#define CONFIG_RT_GROUP_SCHED

//#define CONFIG_SCHED_AUTOGROUP
#define CONFIG_SCHEDSTATS
#define CONFIG_SCHED_INFO

#define CONFIG_NO_HZ_COMMON

//xarray.h
#define CONFIG_BASE_SMALL		0
#define CONFIG_XARRAY_MULTI		0

//CPU Configuration ----------------------------------------
#define CONFIG_CPU_FREQ
#define CONFIG_CPUSETS
#define CONFIG_VIRT_CPU_ACCOUNTING_NATIVE
//kernel/cpu.c
#define CONFIG_HOTPLUG_CPU
#define CONFIG_INIT_ALL_POSSIBLE
#define CONFIG_CPUMASK_OFFSTACK
#define CONFIG_USE_PERCPU_NUMA_NODE_ID

#define CONFIG_NUMA

//#define CONFIG_DEBUG_PREEMPT
//#define CONFIG_PROVE_RCU_LIST


//#define __USE_MISC

#ifdef __cplusplus
}
#endif

#endif // __CONFIG_H
