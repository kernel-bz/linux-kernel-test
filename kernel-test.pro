TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

#linux/time.h sys/time.h
DEFINES += _POSIX_SOURCE HAVE_STRUCT_TIMESPEC __timeval_defined \
        __timespec_defined _SYS_TIME_H __KERNEL__ \
        HAS_CAA_GET_CYCLES UATOMIC_NO_LINK_ERROR

LIBS += -Lpthread

QMAKE_CFLAGS += -w -Wno-unused-parameter -finstrument-functions \
        -Wvariadic-macros

INCLUDEPATH += include/ lib/ lib/traceevent/ scripts/ arch/x86/include/

SOURCES += \
    main.c \
    test/sched/decay_load.c \
    test/sched/update_load_avg.c \
    mm/slab_user.c \
    lib/api/fd/array.c \
    lib/api/fs/fs.c \
    lib/api/debug.c \
    lib/math/div64.c \
    lib/subcmd/exec-cmd.c \
    lib/subcmd/help.c \
    lib/subcmd/pager.c \
    lib/subcmd/parse-options.c \
    lib/subcmd/run-command.c \
    lib/subcmd/sigchain.c \
    lib/subcmd/subcmd-config.c \
    lib/symbol/kallsyms.c \
    lib/argv_split.c \
    lib/bitmap.c \
    lib/ctype.c \
    lib/find_bit.c \
    lib/hweight.c \
    lib/idr.c \
    lib/radix-tree.c \
    lib/rbtree.c \
    lib/str_error_r.c \
    lib/string.c \
    lib/test_ida.c \
    lib/vsprintf.c \
    lib/zalloc.c \
    lib/cpumask.c \
    lib/xarray.c \
    kernel/sched/pelt.c \
    kernel/sched/stats.c \
    kernel/sched/completion.c \
    kernel/sched/cpuacct.c \
    kernel/sched/cpudeadline.c \
    kernel/sched/loadavg.c \
    kernel/sched/stats.c \
    kernel/sched/stop_task.c \
    kernel/sched/deadline.c \
    kernel/sched/rt.c \
    kernel/sched/fair.c \
    kernel/sched/idle.c \
    kernel/sched/core.c \
    kernel/sched/topology.c \
    kernel/sched/cpupri.c \
    init/init_task.c \
    init/main-init.c \
    test/sched/sched-test.c \
    lib/lockdep/common.c \
    test/basic/ptr-test.c \
    test/basic/types-test.c \
    kernel/time/time.c \
    lib/stack_depth.c \
    test/sched/sched-debug.c \
    lib/plist.c \
    kernel/sched/clock.c \
    drivers/of/address.c \
    drivers/of/base.c \
    drivers/of/device.c \
    drivers/of/dynamic.c \
    drivers/of/fdt_address.c \
    drivers/of/irq.c \
    drivers/of/kobj.c \
    drivers/of/of_mdio.c \
    drivers/of/of_net.c \
    drivers/of/of_numa.c \
    drivers/of/of_reserved_mem.c \
    drivers/of/overlay.c \
    drivers/of/pdt.c \
    drivers/of/platform.c \
    drivers/of/property.c \
    drivers/of/resolver.c \
    drivers/base/arch_topology.c \
    drivers/base/cpu.c \
    lib/kasprintf.c \
    lib/kobject.c \
    lib/kobject_uevent.c \
    scripts/dtc/libfdt/fdt_addresses.c \
    scripts/dtc/libfdt/fdt_empty_tree.c \
    scripts/dtc/libfdt/fdt_overlay.c \
    scripts/dtc/libfdt/fdt_ro.c \
    scripts/dtc/libfdt/fdt_rw.c \
    scripts/dtc/libfdt/fdt_strerror.c \
    scripts/dtc/libfdt/fdt_sw.c \
    scripts/dtc/libfdt/fdt_wip.c \
    scripts/dtc/libfdt/fdt.c \
    lib/logic_pio.c \
    lib/api/api-cpu.c \
    drivers/of/of_fdt.c \
    mm/util.c \
    kernel/resource.c \
    kernel/sched/wait.c \
    drivers/base/firmware.c \
    lib/kstrtox.c \
    mm/memblock.c \
    lib/crc32.c \
    lib/sort.c \
    lib/uuid.c \
    lib/hexdump.c \
    kernel/sched/debug_.c \
    kernel/cpu_.c \
    drivers/base/core_.c \
    arch/arm64/mm/numa.c \
    test/basic/cpus-mask-test.c \
    test/config/config.c \
    test/basic/basic.c \
    test/algorithm/algorithm.c \
    test/init/init.c \
    test/algorithm/rbtree03.c \
    test/algorithm/rbtree02.c \
    test/algorithm/rbtree01.c \
    test/algorithm/list04.c \
    test/algorithm/list03.c \
    test/algorithm/list02.c \
    test/algorithm/list01.c \
    lib/test_xarray.c \
    lib/test_list_sort.c \
    lib/list_sort.c \
    lib/random32.c \
    lib/test_sort.c \
    lib/usercopy.c \
    test/basic/struct-test.c \
    test/basic/basic-test.c \
    kernel/power/energy_model.c \
    drivers/base/platform.c \
    drivers/base/driver.c \
    kernel/locking/rwsem.c \
    lib/klist.c \
    drivers/base/bus.c \
    drivers/base/dd.c \
    drivers/base/swnode.c \
    test/user/user-common.c \
    test/user/user-driver.c \
    test/user/user-lock.c \
    test/user/user-sched.c \
    drivers/base/property_.c

HEADERS += \
    include/test/test.h \
    include/linux/list.h \
    include/linux/atomic.h \
    include/linux/bitmap.h \
    include/linux/bitops.h \
    include/linux/bits.h \
    include/linux/bug.h \
    include/linux/compiler-gcc.h \
    include/linux/compiler.h \
    include/linux/const.h \
    include/linux/coresight-pmu.h \
    include/linux/ctype.h \
    include/linux/debug_locks.h \
    include/linux/delay.h \
    include/linux/err.h \
    include/linux/export.h \
    include/linux/filter.h \
    include/linux/ftrace.h \
    include/linux/gfp.h \
    include/linux/hardirq.h \
    include/linux/hash.h \
    include/linux/hashtable.h \
    include/linux/interrupt.h \
    include/linux/irqflags.h \
    include/linux/jhash.h \
    include/linux/kallsyms.h \
    include/linux/kern_levels.h \
    include/linux/kernel.h \
    include/linux/linkage.h \
    include/linux/list.h \
    include/linux/lockdep.h \
    include/linux/log2.h \
    include/linux/module.h \
    include/linux/mutex.h \
    include/linux/nmi.h \
    include/linux/numa.h \
    include/linux/overflow.h \
    include/linux/poison.h \
    include/linux/proc_fs.h \
    include/linux/rbtree_augmented.h \
    include/linux/rbtree.h \
    include/linux/rcu.h \
    include/linux/refcount.h \
    include/linux/ring_buffer.h \
    include/linux/seq_file.h \
    include/linux/sizes.h \
    include/linux/spinlock.h \
    include/linux/stacktrace.h \
    include/linux/string.h \
    include/linux/stringify.h \
    include/linux/time64.h \
    include/linux/zalloc.h \
    include/asm/alternative-asm.h \
    include/asm/atomic.h \
    include/asm/barrier.h \
    include/asm/bug.h \
    include/asm/export.h \
    include/asm/sections.h \
    include/asm-generic/bitops/__ffs.h \
    include/asm-generic/bitops/__ffz.h \
    include/asm-generic/bitops/__fls.h \
    include/asm-generic/bitops/arch_hweight.h \
    include/asm-generic/bitops/atomic.h \
    include/asm-generic/bitops/const_hweight.h \
    include/asm-generic/bitops/find.h \
    include/asm-generic/bitops/fls.h \
    include/asm-generic/bitops/fls64.h \
    include/asm-generic/bitops/hweight.h \
    include/asm-generic/bitops/non-atomic.h \
    include/asm-generic/atomic-gcc.h \
    include/asm-generic/barrier.h \
    include/asm-generic/bitops.h \
    include/asm-generic/bitsperlong.h \
    include/asm-generic/hugetlb_encode.h \
    include/linux/sched/clock.h \
    include/linux/sched/task.h \
    include/linux/unaligned/packed_struct.h \
    include/uapi/asm/bitsperlong.h \
    include/uapi/asm/bpf_perf_event.h \
    include/uapi/asm/errno.h \
    include/uapi/asm-generic/bitsperlong.h \
    include/uapi/asm-generic/bpf_perf_event.h \
    include/uapi/asm-generic/errno-base.h \
    include/uapi/asm-generic/errno.h \
    include/uapi/asm-generic/fcntl.h \
    include/uapi/asm-generic/ioctls.h \
    include/uapi/asm-generic/mman-common-tools.h \
    include/uapi/asm-generic/mman-common.h \
    include/uapi/asm-generic/mman.h \
    include/uapi/asm-generic/socket.h \
    include/uapi/asm-generic/unistd.h \
    include/uapi/drm/drm.h \
    include/uapi/drm/i915_drm.h \
    include/uapi/linux/tc_act/tc_bpf.h \
    include/uapi/linux/bpf.h \
    include/uapi/linux/bpf_common.h \
    include/uapi/linux/bpf_perf_event.h \
    include/uapi/linux/btf.h \
    include/uapi/linux/const.h \
    include/uapi/linux/erspan.h \
    include/uapi/linux/ethtool.h \
    include/uapi/linux/fadvise.h \
    include/uapi/linux/fcntl.h \
    include/uapi/linux/fs.h \
    include/uapi/linux/fscrypt.h \
    include/uapi/linux/hw_breakpoint.h \
    include/uapi/linux/if_link.h \
    include/uapi/linux/if_tun.h \
    include/uapi/linux/if_xdp.h \
    include/uapi/linux/in.h \
    include/uapi/linux/kcmp.h \
    include/uapi/linux/kvm.h \
    include/uapi/linux/lirc.h \
    include/uapi/linux/mman.h \
    include/uapi/linux/mount.h \
    include/uapi/linux/netlink.h \
    include/uapi/linux/perf_event.h \
    include/uapi/linux/pkt_cls.h \
    include/uapi/linux/pkt_sched.h \
    include/uapi/linux/prctl.h \
    include/uapi/linux/sched.h \
    include/uapi/linux/seg6.h \
    include/uapi/linux/seg6_local.h \
    include/uapi/linux/stat.h \
    include/uapi/linux/tls.h \
    include/uapi/linux/usbdevice_fs.h \
    include/uapi/linux/vhost.h \
    include/uapi/sound/asound.h \
    include/test/config.h \
    include/linux/sched.h \
    include/linux/cache.h \
    include/asm-generic/cache.h \
    include/linux/compiler_types.h \
    include/linux/cpumask.h \
    include/linux/threads.h \
    include/linux/restart_block.h \
    include/linux/pid.h \
    include/linux/rculist.h \
    include/linux/wait.h \
    include/linux/spinlock_types_up.h \
    include/linux/spinlock_types.h \
    include/linux/rwlock.h \
    include/linux/rwlock_types.h \
    include/linux/slab.h \
    include/asm-generic/word-at-a-time.h \
    include/asm-generic/page.h \
    include/asm-generic/memory_model.h \
    include/asm-generic/getorder.h \
    include/linux/mm.h \
    include/linux/pfn.h \
    kernel/sched/sched.h \
    kernel/sched/pelt.h \
    kernel/sched/sched-pelt.h \
    include/linux/seqlock.h \
    include/linux/llist.h \
    include/linux/plist.h \
    include/linux/sched/prio.h \
    include/linux/sched/sysctl.h \
    include/linux/math64.h \
    include/asm-generic/div64.h \
    include/linux/kernel_stat.h \
    include/linux/sched/autogroup.h \
    include/linux/radix-tree.h \
    include/linux/radix-tree-user.h \
    kernel/sched/stats.h \
    include/test/debug.h \
    include/linux/sched/topology.h \
    include/linux/topology.h \
    include/linux/arch_topology.h \
    include/linux/sched/idle.h \
    include/linux/sched/clock.h \
    include/linux/sched/coredump.h \
    include/linux/sched/cpufreq.h \
    include/linux/sched/deadline.h \
    include/linux/sched/debug.h \
    include/linux/sched/hotplug.h \
    include/linux/sched/idle.h \
    include/linux/sched/init.h \
    include/linux/sched/isolation.h \
    include/linux/sched/jobctl.h \
    include/linux/sched/loadavg.h \
    include/linux/sched/nohz.h \
    include/linux/sched/numa_balancing.h \
    include/linux/sched/prio.h \
    include/linux/sched/rt.h \
    include/linux/sched/smt.h \
    include/linux/sched/stat.h \
    include/linux/sched/sysctl.h \
    include/linux/sched/task_stack.h \
    include/linux/sched/task.h \
    include/linux/sched/topology.h \
    include/linux/sched/types.h \
    include/linux/sched/user.h \
    include/linux/sched/wake_q.h \
    include/linux/sched/xacct.h \
    include/uapi/linux/sched/types.h \
    kernel/sched/autogroup.h \
    kernel/sched/cpudeadline.h \
    kernel/sched/cpupri.h \
    kernel/sched/features.h \
    include/test/define-usr.h \
    include/linux/smp.h \
    include/linux/thread_info.h \
    include/linux/smp.h \
    include/asm-generic/current.h \
    include/asm/thread_info.h \
    include/asm/current.h \
    include/linux/seqlock.h \
    include/linux/types-user.h \
    include/linux/ktime.h \
    include/linux/jiffies.h \
    include/generated/timeconst.h \
    include/linux/typecheck.h \
    include/linux/rwlock_types.h \
    include/linux/hrtimer.h \
    include/linux/sched_clock.h \
    include/linux/errno.h \
    include/uapi/linux/errno.h \
    include/linux/completion.h \
    include/linux/kernel_stat.h \
    include/linux/cpuhotplug.h \
    include/linux/swait.h \
    lib/api/fd/array.h \
    lib/api/fs/fs.h \
    lib/api/fs/tracing_path.h \
    lib/api/cpu.h \
    lib/api/debug-internal.h \
    lib/api/debug.h \
    lib/bpf/btf.h \
    lib/bpf/hashmap.h \
    lib/bpf/libbpf.h \
    lib/bpf/libbpf_internal.h \
    lib/bpf/libbpf_util.h \
    lib/bpf/nlattr.h \
    lib/bpf/str_error.h \
    lib/bpf/xsk.h \
    lib/lockdep/include/liblockdep/common.h \
    lib/lockdep/include/liblockdep/mutex.h \
    lib/lockdep/include/liblockdep/rwlock.h \
    lib/lockdep/tests/common.h \
    lib/lockdep/lockdep_internals.h \
    lib/lockdep/lockdep_states.h \
    lib/subcmd/exec-cmd.h \
    lib/subcmd/help.h \
    lib/subcmd/pager.h \
    lib/subcmd/parse-options.h \
    lib/subcmd/run-command.h \
    lib/subcmd/sigchain.h \
    lib/subcmd/subcmd-config.h \
    lib/subcmd/subcmd-util.h \
    lib/symbol/kallsyms.h \
    include/linux/btf.h \
    include/linux/nospec.h \
    include/linux/kref.h \
    include/linux/rwsem.h \
    include/linux/init_task.h \
    include/linux/rcupdate.h \
    include/linux/jump_label.h \
    include/uapi/asm-generic/setup.h \
    include/linux/cpu.h \
    include/linux/mutex.h \
    include/linux/memblock.h \
    include/linux/profile.h \
    include/test/basic.h \
    test/basic/type01.h \
    test/basic/type02.h \
    test/basic/type03.h \
    test/basic/type-limits.h \
    include/linux/cgroup-defs.h \
    include/linux/cgroup.h \
    include/linux/cgroup_subsys.h \
    include/linux/kconfig.h \
    include/asm-generic/percpu.h \
    include/linux/percpu-defs.h \
    include/linux/percpu.h \
    include/asm-generic/param.h \
    include/uapi/asm-generic/param.h \
    include/uapi/linux/time_types.h \
    include/linux/time32.h \
    include/linux/uaccess.h \
    include/uapi/asm-generic/posix_types.h \
    include/asm-generic/atomic-long.h \
    include/linux/limits.h \
    include/uapi/linux/limits.h \
    include/linux/sched/signal.h \
    include/asm-generic/preempt.h \
    include/asm-generic/switch_to.h \
    include/linux/nodemask.h \
    drivers/of/of_private.h \
    include/linux/of_address.h \
    include/linux/of_clk.h \
    include/linux/of_device.h \
    include/linux/of_dma.h \
    include/linux/of_fdt.h \
    include/linux/of_gpio.h \
    include/linux/of_graph.h \
    include/linux/of_iommu.h \
    include/linux/of_irq.h \
    include/linux/of_mdio.h \
    include/linux/of_net.h \
    include/linux/of_pci.h \
    include/linux/of_pdt.h \
    include/linux/of_platform.h \
    include/linux/of_reserved_mem.h \
    include/linux/of.h \
    include/linux/device.h \
    include/linux/platform_device.h \
    include/linux/ioport.h \
    include/linux/kobject.h \
    include/linux/kobject_ns.h \
    include/linux/workqueue.h \
    include/linux/klist.h \
    include/linux/uidgid.h \
    include/linux/highuid.h \
    include/linux/sort.h \
    include/linux/cpuset.h \
    kernel/workqueue_internal.h \
    include/linux/io.h \
    include/linux/logic_pio.h \
    include/linux/fwnode.h \
    include/asm-generic/device.h \
    include/linux/kconfig.h \
    include/linux/pm.h \
    include/linux/mod_devicetable.h \
    include/linux/uuid.h \
    include/uapi/linux/uuid.h \
    include/linux/notifier.h \
    include/linux/irqdomain.h \
    include/linux/irqhandler.h \
    include/linux/phy.h \
    include/linux/phy_fixed.h \
    include/linux/mdio.h \
    include/uapi/linux/mdio.h \
    include/uapi/linux/kernel.h \
    include/linux/cpufreq.h \
    include/asm-generic/atomic64.h \
    include/asm-generic/io.h \
    drivers/base/base.h \
    include/linux/sysfs.h \
    include/asm-generic/signal.h \
    include/uapi/asm-generic/signal.h \
    scripts/dtc/libfdt/fdt.h \
    scripts/dtc/libfdt/libfdt_env.h \
    scripts/dtc/libfdt/libfdt_internal.h \
    scripts/dtc/libfdt/libfdt.h \
    include/linux/libfdt_env.h \
    include/linux/libfdt.h \
    include/linux/resource.h \
    include/linux/resource_ext.h \
    include/uapi/linux/resource.h \
    include/test/define-usr-dev.h \
    include/test/define-usr-lock.h \
    lib/kstrtox.h \
    include/asm-generic/bug.h \
    include/linux/build_bug.h \
    include/linux/kmod.h \
    include/linux/crc32.h \
    include/linux/crc32c.h \
    include/linux/crc32poly.h \
    lib/crc32defs.h \
    include/linux/compiler_attributes.h \
    arch/arm64/include/asm/numa_.h \
    include/asm-generic/timex.h \
    include/linux/random_.h \
    include/uapi/linux/random.h \
    include/linux/once.h \
    include/linux/list_sort.h \
    include/linux/clockchips.h \
    include/asm-generic/uaccess.h \
    include/asm-generic/cmpxchg.h \
    include/asm-generic/cmpxchg-local.h \
    arch/x86/include/asm/msr.h \
    arch/x86/include/asm/tsc.h \
    include/uapi/asm/msr.h \
    arch/x86/include/asm/asm.h \
    arch/x86/include/asm/cpumask.h \
    arch/x86/include/asm/msr-index.h \
    include/linux/energy_model.h \
    include/linux/kthread.h \
    include/linux/irq_work.h \
    include/linux/timer.h \
    include/linux/moduleparam.h \
    include/asm-generic/module.h \
    include/linux/property.h \
    include/linux/pm_runtime.h \
    include/linux/pm_domain.h \
    include/linux/acpi.h \
    include/acpi/platform/acenv.h \
    include/acpi/platform/acenvex.h \
    include/acpi/platform/acgcc.h \
    include/acpi/platform/acgccex.h \
    include/acpi/platform/acintel.h \
    include/acpi/platform/aclinux.h \
    include/acpi/platform/aclinuxex.h \
    include/acpi/acbuffer.h \
    include/acpi/acconfig.h \
    include/acpi/acexcep.h \
    include/acpi/acnames.h \
    include/acpi/acoutput.h \
    include/acpi/acpi.h \
    include/acpi/acpi_bus.h \
    include/acpi/acpi_drivers.h \
    include/acpi/acpi_io.h \
    include/acpi/acpi_lpat.h \
    include/acpi/acpi_numa.h \
    include/acpi/acpiosxf.h \
    include/acpi/acpixf.h \
    include/acpi/acrestyp.h \
    include/acpi/actbl.h \
    include/acpi/actbl1.h \
    include/acpi/actbl2.h \
    include/acpi/actbl3.h \
    include/acpi/actypes.h \
    include/acpi/acuuid.h \
    include/acpi/apei.h \
    include/acpi/battery.h \
    include/acpi/button.h \
    include/acpi/cppc_acpi.h \
    include/acpi/ghes.h \
    include/acpi/hed.h \
    include/acpi/nfit.h \
    include/acpi/pcc.h \
    include/acpi/pdc_intel.h \
    include/acpi/processor.h \
    include/acpi/reboot.h \
    include/acpi/video.h \
    include/linux/stat.h \
    include/linux/init.h \
    include/linux/async.h \
    drivers/base/power/power.h \
    include/test/user.h
    include/linux/percpu-rwsem.h \

DISTFILES += \
    lib/bpf/libbpf.a \
    docs/study/study-history.txt \
    drivers/of/unittest-data/overlay.dts \
    drivers/of/unittest-data/overlay_0.dts \
    drivers/of/unittest-data/overlay_1.dts \
    drivers/of/unittest-data/overlay_10.dts \
    drivers/of/unittest-data/overlay_11.dts \
    drivers/of/unittest-data/overlay_12.dts \
    drivers/of/unittest-data/overlay_13.dts \
    drivers/of/unittest-data/overlay_15.dts \
    drivers/of/unittest-data/overlay_2.dts \
    drivers/of/unittest-data/overlay_3.dts \
    drivers/of/unittest-data/overlay_4.dts \
    drivers/of/unittest-data/overlay_5.dts \
    drivers/of/unittest-data/overlay_6.dts \
    drivers/of/unittest-data/overlay_7.dts \
    drivers/of/unittest-data/overlay_8.dts \
    drivers/of/unittest-data/overlay_9.dts \
    drivers/of/unittest-data/overlay_bad_add_dup_node.dts \
    drivers/of/unittest-data/overlay_bad_add_dup_prop.dts \
    drivers/of/unittest-data/overlay_bad_phandle.dts \
    drivers/of/unittest-data/overlay_bad_symbol.dts \
    drivers/of/unittest-data/overlay_base.dts \
    drivers/of/unittest-data/testcases.dts \
    drivers/of/unittest-data/tests-interrupts.dtsi \
    drivers/of/unittest-data/tests-match.dtsi \
    drivers/of/unittest-data/tests-overlay.dtsi \
    drivers/of/unittest-data/tests-phandle.dtsi \
    drivers/of/unittest-data/tests-platform.dtsi \
    docs/build/build_log_20201127.txt \
    docs/study/cpu-topo-domain.txt
