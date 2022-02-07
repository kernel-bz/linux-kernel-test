#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct test_s {
        char a;
        int b;
};
static struct test_s ts = {
        .a = 10,
        .b = 20,
};

typedef struct maple_enode *maple_enode; /* encoded node */
typedef struct maple_pnode *maple_pnode; /* parent node */


int main(void)
{
    struct maple_enode *pe;
    struct maple_pnode *pp;

    maple_enode pe2;

    pe = (maple_enode)&ts;
    pe2 = (maple_enode)pe;
    pp = (maple_pnode)pe2;

    printf("size=%ld\n", sizeof(pe));
    printf("%p, %p, %p, %p\n", &ts, pe, pe2, pp);

    //printf("%d, %d\n", pp->a, pp->b);

    return 0;
}
