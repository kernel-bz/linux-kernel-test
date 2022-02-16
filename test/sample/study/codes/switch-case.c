#include <stdio.h>

static int scase1(int type)
{
        switch (type) {
                case 0:
                        return 0;
                case 1:
                        return 1;
                default:
                        return 3;
        }
}

static inline int scase2(int type)
{
        switch (type) {
                case 0:
                        return 0;
                case 1:
                case 2:
                        return 2;
                case 3:
                        return 3;
        }
}

int main(void)
{
        int ret;

        ret = scase1(1);
        printf("ret=%d\n", ret);
        ret = scase2(2);
        printf("ret=%d\n", ret);

        return 0;
}
