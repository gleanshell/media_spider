//
// Created by xx on 2018/7/27.
//
//
// Created by xx on 2018/12/30.
//

#ifndef BT_PARSER_H
#define BT_PARSER_H
#endif // BT_PARSER_H

typedef unsigned long long u_64;
typedef unsigned long u_32;



#define MAX_STACK_SIZE 100

typedef struct
{
    char stack_array[MAX_STACK_SIZE];
    int stack_size;
} stack_t;

enum
{
    BEGIN = 0,
    STRING = 1,
    DICT,
    DICT_KEY,
    DICT_VAL,
    LIST_HEAD,
    LIST_ELE,
    BUTT
};



enum
{
    DICT_WAIT_KEY = 1,
    DICT_WAIT_VAL
};

#define MAX_STR_LEN 100

typedef struct str_ele
{
    char str[MAX_STR_LEN];
    int str_type;
    struct
    {
        struct str_ele *dict_val_ref;
        struct str_ele *list_next_ref;
    }p;

} str_ele_t;

typedef struct
{
    str_ele_t e[1000];
    int ele_cnt;
}ben_dict_t;

typedef struct
{
    str_ele_t *stack_array[MAX_STACK_SIZE];
    int stack_size;
} contex_stack_t;

int ben_coding(char*b, u_32 len,u_32 *pos);
void print_result(stack_t *s, contex_stack_t *c);
