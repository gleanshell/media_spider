#include "list.h"
#include <stdlib.h>
#include <stdio.h>

list_head_t lst;
list_head_t *l=&lst;
int mainlist()
{
    list_init(l);
    for (int i = 0; i < 100; ++i)
    {
        test_t *tmp = (test_t*)malloc(sizeof(test_t));
        tmp->data = i;
        list_add_tail(l, &tmp->node);
    }
    list_head_t*t;
    list_for_each(l,t)
    {
        test_t *x = container_of(test_t, node, t);
        printf("go throuth list: (%d)\n", x->data);
    }

    return 0;
}
