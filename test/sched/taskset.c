/*
 * process test
 * taskset -c <n> <name> &
 * taskset -cp <n> <pid>
 * renice -n -1 -p <pid>
 * /sys/fs/cgroup/cpu/A echo <pid> > tasks
 * htop
 * /proc/sched_debug
 * /proc/<pid>/sched
 * /proc/<pid>/cgroup
 */

int taskset_test(void)
{
    unsigned long	cnt=0;

    while (1) {
        cnt++;
    }
    return 0;
}
