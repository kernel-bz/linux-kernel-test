#ifndef __TEST_CONFIG_H
#define __TEST_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(HAVE_STRUCT_TIMESPEC)
#define HAVE_STRUCT_TIMESPEC
#endif
//#if !defined(_TIMESPEC_DEFINED)

#define CONFIG_NR_CPUS		4
#define CONFIG_SMP

#define CONFIG_CGROUP_SCHED
#define CONFIG_FAIR_GROUP_SCHED
#define CONFIG_CFS_BANDWIDTH
//#define CONFIG_SCHED_AUTOGROUP


//xarray.h
#define CONFIG_BASE_SMALL		0
#define CONFIG_XARRAY_MULTI		0

#define CONFIG_CPU_FREQ
#define CONFIG_CPUSETS
#define CONFIG_VIRT_CPU_ACCOUNTING_NATIVE

#define CONFIG_THREAD_INFO_IN_TASK

#define CONFIG_DEBUG_PREEMPT


//#define __USE_MISC

#ifdef __cplusplus
}
#endif

#endif // __CONFIG_H