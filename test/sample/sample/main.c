#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

//#define CONFIG_64BIT

//#define nr_cpu_ids	16
//#define nr_cpu_ids	1024
//#define nr_cpu_ids	65536
//#define nr_cpu_ids	4194304
#define nr_cpu_ids	524288
#define NR_CPUS		nr_cpu_ids

# ifdef CONFIG_64BIT
# define RCU_FANOUT 64
# else
# define RCU_FANOUT 32
# endif

#define RCU_FANOUT_LEAF 16

//32: 16,  512, 16384,  524288
//64: 16, 1024, 65536, 4194304
#define RCU_FANOUT_1	      	(RCU_FANOUT_LEAF)
#define RCU_FANOUT_2	      	(RCU_FANOUT_1 * RCU_FANOUT)
#define RCU_FANOUT_3	      	(RCU_FANOUT_2 * RCU_FANOUT)
#define RCU_FANOUT_4	      	(RCU_FANOUT_3 * RCU_FANOUT)

#define DIV_ROUND_UP(n, d)		(((n) + (d) - 1) / (d))

#if NR_CPUS <= RCU_FANOUT_1
#  define RCU_NUM_LVLS	      1
#  define NUM_RCU_LVL_0	      1
#  define NUM_RCU_NODES	      NUM_RCU_LVL_0
#  define NUM_RCU_LVL_INIT    { NUM_RCU_LVL_0 }
#  define RCU_NODE_NAME_INIT  { "rcu_node_0" }
#  define RCU_FQS_NAME_INIT   { "rcu_node_fqs_0" }
#elif NR_CPUS <= RCU_FANOUT_2
#  define RCU_NUM_LVLS	      2
#  define NUM_RCU_LVL_0	      1
#  define NUM_RCU_LVL_1	      DIV_ROUND_UP(NR_CPUS, RCU_FANOUT_1)
#  define NUM_RCU_NODES	      (NUM_RCU_LVL_0 + NUM_RCU_LVL_1)
#  define NUM_RCU_LVL_INIT    { NUM_RCU_LVL_0, NUM_RCU_LVL_1 }
#  define RCU_NODE_NAME_INIT  { "rcu_node_0", "rcu_node_1" }
#  define RCU_FQS_NAME_INIT   { "rcu_node_fqs_0", "rcu_node_fqs_1" }
#elif NR_CPUS <= RCU_FANOUT_3
#  define RCU_NUM_LVLS	      3
#  define NUM_RCU_LVL_0	      1
#  define NUM_RCU_LVL_1	      DIV_ROUND_UP(NR_CPUS, RCU_FANOUT_2)
#  define NUM_RCU_LVL_2	      DIV_ROUND_UP(NR_CPUS, RCU_FANOUT_1)
#  define NUM_RCU_NODES	      (NUM_RCU_LVL_0 + NUM_RCU_LVL_1 + NUM_RCU_LVL_2)
#  define NUM_RCU_LVL_INIT    { NUM_RCU_LVL_0, NUM_RCU_LVL_1, NUM_RCU_LVL_2 }
#  define RCU_NODE_NAME_INIT  { "rcu_node_0", "rcu_node_1", "rcu_node_2" }
#  define RCU_FQS_NAME_INIT   { "rcu_node_fqs_0", "rcu_node_fqs_1", "rcu_node_fqs_2" }
#elif NR_CPUS <= RCU_FANOUT_4
#  define RCU_NUM_LVLS	      4
#  define NUM_RCU_LVL_0	      1
#  define NUM_RCU_LVL_1	      DIV_ROUND_UP(NR_CPUS, RCU_FANOUT_3)
#  define NUM_RCU_LVL_2	      DIV_ROUND_UP(NR_CPUS, RCU_FANOUT_2)
#  define NUM_RCU_LVL_3	      DIV_ROUND_UP(NR_CPUS, RCU_FANOUT_1)
#  define NUM_RCU_NODES	      (NUM_RCU_LVL_0 + NUM_RCU_LVL_1 + NUM_RCU_LVL_2 + NUM_RCU_LVL_3)
#  define NUM_RCU_LVL_INIT    { NUM_RCU_LVL_0, NUM_RCU_LVL_1, NUM_RCU_LVL_2, NUM_RCU_LVL_3 }
#  define RCU_NODE_NAME_INIT  { "rcu_node_0", "rcu_node_1", "rcu_node_2", "rcu_node_3" }
#  define RCU_FQS_NAME_INIT   { "rcu_node_fqs_0", "rcu_node_fqs_1", "rcu_node_fqs_2", "rcu_node_fqs_3" }
#else
# error "CONFIG_RCU_FANOUT insufficient for NR_CPUS"
#endif /* #if (NR_CPUS) <= RCU_FANOUT_1 */

bool rcu_fanout_exact;
int rcu_fanout_leaf = RCU_FANOUT_LEAF;
int rcu_num_lvls = RCU_NUM_LVLS;
int num_rcu_lvl[] = NUM_RCU_LVL_INIT;
int rcu_num_nodes = NUM_RCU_NODES;


//kernel/rcu/rcu.h
static inline void rcu_init_levelspread(int *levelspread, const int *levelcnt)
{
    int i;

    if (rcu_fanout_exact) {
        levelspread[rcu_num_lvls - 1] = rcu_fanout_leaf;
        for (i = rcu_num_lvls - 2; i >= 0; i--)
            levelspread[i] = RCU_FANOUT;
    } else {
        int ccur;
        int cprv;

        cprv = nr_cpu_ids;
        for (i = rcu_num_lvls - 1; i >= 0; i--) {
            ccur = levelcnt[i];
            levelspread[i] = (cprv + ccur - 1) / ccur;
            cprv = ccur;
        }
    }
}

int main()
{
    int i, j;
    int levelspread[RCU_NUM_LVLS];
    int cpustride = 1;

    printf("RCU_FANOUT_LEAF : %d\n", RCU_FANOUT_LEAF);
    printf("RCU_FANOUT      : %d\n", RCU_FANOUT);
    printf("RCU_NUM_LVLS    : %d\n", RCU_NUM_LVLS);
    printf("NUM_RCU_NODES   : %d\n", NUM_RCU_NODES);

    printf("RCU_FANOUT_# : %d, %d, %d, %d\n"
           , RCU_FANOUT_1, RCU_FANOUT_2, RCU_FANOUT_3, RCU_FANOUT_4);

    printf("rcu_num_lvl : ");
    for (i = 0; i < RCU_NUM_LVLS; i++)
        printf("%d, ", num_rcu_lvl[i]);
    printf("\n");

    rcu_fanout_exact = false;
    rcu_init_levelspread(levelspread, num_rcu_lvl);

    printf("levelspread : ");
    for (i = 0; i < RCU_NUM_LVLS; i++)
        printf("%d, ", levelspread[i]);
    printf("\n");

#if 0
    rcu_fanout_exact = true;
    rcu_init_levelspread(levelspread, num_rcu_lvl);

    printf("levelspread : ");
    for (i = 0; i < RCU_NUM_LVLS; i++)
        printf("%d, ", levelspread[i]);
    printf("\n");
#endif

    int cnt = 0;
    int grpnum, parent, old = -1;

    for (i = rcu_num_lvls - 1; i >= 0; i--) {
        cpustride *= levelspread[i];
        printf("cpustride=%d\n", cpustride);

        for (j = 0; j < num_rcu_lvl[i]; j++) {
            grpnum = j % levelspread[i - 1];
            parent = j / levelspread[i - 1];
            if (parent != old) {
                //printf("[%d, %d] : %d, %d\n", i, j, grpnum, parent);
            }
            old = parent;
            cnt++;
        }

    }
    printf("cnt=%d\n", cnt);	//nodes

    return 0;
}
