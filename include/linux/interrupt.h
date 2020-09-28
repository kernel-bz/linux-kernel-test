#ifndef _TOOLS_INCLUDE_LINUX_INTERRUPT_H
#define _TOOLS_INCLUDE_LINUX_INTERRUPT_H


enum
{
    HI_SOFTIRQ=0,
    TIMER_SOFTIRQ,
    NET_TX_SOFTIRQ,
    NET_RX_SOFTIRQ,
    BLOCK_SOFTIRQ,
    IRQ_POLL_SOFTIRQ,
    TASKLET_SOFTIRQ,
    SCHED_SOFTIRQ,
    HRTIMER_SOFTIRQ, /* Unused, but kept as tools rely on the
                numbering. Sigh! */
    RCU_SOFTIRQ,    /* Preferable RCU should always be the last softirq */

    NR_SOFTIRQS
};

#define SOFTIRQ_STOP_IDLE_MASK (~(1 << RCU_SOFTIRQ))


#endif /* _TOOLS_INCLUDE_LINUX_INTERRUPT_H */
