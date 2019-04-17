//
// Created by xx on 2018/7/27.
//

//
// Created by xx on 2018/12/30.
//

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <clocale>
#include <afxres.h>
//#include <cwchar>

#include "bt_parser.h"


void print_result(stack_t *s, contex_stack_t *c);
stack_t g_stack;
contex_stack_t g_ctx_stack;
ben_dict_t dict;
int status = DICT_WAIT_KEY;

char str_desc[BUTT+1][10]={
        "BEGIN",
        "STRING",
        "DICT",
        "DICT_KEY",
        "DICT_VAL",
        "LIST_HEAD",
        "LIST_ELE",
        "BUTT"
};

int print_flag =0;

int push(stack_t *s, char a)
{
    if (s->stack_size >= MAX_STACK_SIZE)
    {
        return -1;
    }
    s->stack_array[s->stack_size++] = a;
    return 0;
}

int pop(stack_t *s)
{
    if (s->stack_size <= 0)
    {
        return -1;
    }
    s->stack_size --;
    return s->stack_array[s->stack_size];
}

int peek(stack_t *s)
{
    if (s->stack_size <= 0)
    {
        return -1;
    }
    return s->stack_array[s->stack_size-1];
}

void init_stack(stack_t *s)
{
    memset(s, 0 , sizeof(stack_t));
}
/***************************************for ctx******************************************/
int ctx_push(contex_stack_t *s, str_ele_t** a)
{
    if (s->stack_size >= MAX_STACK_SIZE)
    {
        return -1;
    }
    s->stack_array[s->stack_size++] = *a;
    return 0;
}

int ctx_pop(contex_stack_t *s, str_ele_t **ele)
{
    if (s->stack_size <= 0)
    {
        return -1;
    }
    s->stack_size --;
    *ele = s->stack_array[s->stack_size];
    return 0;
}

int ctx_peek(contex_stack_t *s, str_ele_t **ele)
{
    if (s->stack_size <= 0)
    {
        return -1;
    }
    *ele = s->stack_array[s->stack_size-1];
    return 0;
}

void init_ctx_stack(contex_stack_t *s)
{
    memset(s, 0 , sizeof(contex_stack_t));
}
/**************************************************************************************/
void read_until_char(char *b, u_32 len, u_32 *pos, char a, char buffer[], int *read_len)
{
    int i = 0;
    char *tmp = b+*pos;
    while(1)
    {
        if(*pos >= len)
        {
            break;
        }
        if (a == *tmp)
        {
            (*pos) ++;
            break;
        }
        buffer[i++]=*tmp;
        tmp ++;
        (*pos) ++;
    }
    *read_len = i;
}

int read_n_char(char *b, u_32 len, u_32 *pos, u_32 n, char buf[500])
{
    if (*pos >= len || *pos + n > len)
    {
        return -1;
    }
    memcpy(buf, (*pos+b),n);
    *pos += n;
    return n;
}



int process_a_string(char*b,u_32 len,u_32 *pos,stack_t*s, int str_type, ben_dict_t *dict, contex_stack_t *c)
{
    int ret = -1;
    int read_len = -1;
    char buf[200] = {0};
    char *tmp_big_buf = NULL;
    read_until_char(b, len, pos,':',buf,&read_len);
    if (read_len <= 0)
    {
        printf("read ':' failed,read len:%d, len:(%u), pos:(%u).\n", read_len, len, *pos);
        return -1;
    }
    //read key
    u_32 str_len = strtol(buf,NULL,10);

    tmp_big_buf = (char*)malloc(str_len + 1);
    if (NULL == tmp_big_buf)
    {
        return -1;
    }

    memset(tmp_big_buf, 0, str_len + 1);
    ret = read_n_char(b,len,pos,str_len,tmp_big_buf);
    if (ret <= 0)
    {
        printf("read str failedddd.\n");
        free(tmp_big_buf);
        return -1;
    }

    if (str_type == DICT_KEY)
    {
        str_ele_t * tmp_ele_1 = NULL;
        ctx_peek(c, &tmp_ele_1);

        if (NULL == tmp_ele_1)
        {
            return -1;
            printf("can't be here!\n");
        }

        int n = dict->ele_cnt;
        dict->e[n].str_type = DICT_KEY;
        dict->e[n].str_len = str_len;
        memcpy(dict->e[n].str, tmp_big_buf, str_len+1 );
        str_ele_t * tmp_ele= &dict->e[n];
        dict->ele_cnt ++;
        //list contains dict,the inner dict use val_ref to list dict's inner key-value
        /*
        4:info
        d
            5:files
            l
                d
                    6:length
                    i2660662962e

                    4:path
                    l
                        46:Pacific.Rim.2013.1080p.WEB-DL.H264.AAC-HQC.mp4
                    e

        */
        tmp_ele->p.list_next_ref =tmp_ele_1->p.dict_val_ref;
        tmp_ele_1->p.dict_val_ref = tmp_ele;

        ctx_push(c,&tmp_ele);

    }
    else if (str_type == DICT_VAL)
    {

        int n = dict->ele_cnt;
        dict->e[n].str_type = DICT_VAL;
        dict->e[n].str_len = str_len;
        //memcpy(dict->e[n].str, tmp_big_buf, (str_len+1) > MAX_STR_LEN? MAX_STR_LEN: str_len+1);
        memcpy(dict->e[n].str, tmp_big_buf, (str_len+1) );

        str_ele_t * tmp_ele= &dict->e[n];
        dict->ele_cnt ++;

        str_ele * tmp_ele_1 = NULL;
        ctx_peek(c, &tmp_ele_1);
        if (NULL == tmp_ele_1 || tmp_ele_1->str_type != DICT_KEY )
        {
            printf("err occur..DICT VAL (%d), str = {%s}\n", tmp_ele_1->str_type, tmp_ele_1->str);
            return -1;
        }

        tmp_ele_1->p.dict_val_ref = (struct str_ele *)tmp_ele;
        ctx_pop(c,&tmp_ele_1);

        tmp_ele_1 = NULL;
        ctx_peek(c, &tmp_ele_1);

    }
    else if (str_type == LIST_ELE)
    {
        int n = dict->ele_cnt;
        dict->e[n].str_type = DICT_VAL;
        dict->e[n].str_len = str_len;
        dict->e[n].p.list_next_ref = NULL;
        memcpy(dict->e[n].str, tmp_big_buf, str_len+1 );

        str_ele_t * tmp_ele= &dict->e[n];
        dict->ele_cnt ++;

        str_ele * tmp_ele_1 = NULL;
        ctx_peek(c, &tmp_ele_1);
        if (NULL == tmp_ele_1 )
        {
            printf("err occur.list.(%d)--%d\n", NULL == tmp_ele_1, tmp_ele_1->str_type);
            return -1;
        }

        tmp_ele->p.list_next_ref =tmp_ele_1->p.dict_val_ref;
        tmp_ele_1->p.dict_val_ref = tmp_ele;

    }

    free(tmp_big_buf);
    return 0;
}

int process_a_int(char *b,u_32 len, u_32*pos, stack_t*s, ben_dict_t *dict,contex_stack_t *c)
{
    int read_len = -1;
    int ret = -1;
    char buf[20] = {0};
    read_until_char(b, len, pos,'e',buf,&read_len);
    if (read_len <= 0)
    {
        printf("read ':' failed.\n");
        return -1;
    }

    int n = dict->ele_cnt;
    dict->e[n].str_type = DICT_VAL;
    memcpy(dict->e[n].str, buf, read_len+1 );
    dict->e[n].str_len = read_len;

    str_ele_t * tmp_ele= &dict->e[n];
    dict->ele_cnt ++;

    str_ele * tmp_ele_1 = NULL;
    ctx_peek(c, &tmp_ele_1);
    if (NULL == tmp_ele_1)
    {
        printf("err occur.,INT,%d\n", tmp_ele_1->str_type);
        return -1;
    }
    if (tmp_ele_1->str_type == DICT_KEY)
    {
        ctx_pop(c,&tmp_ele_1);
        tmp_ele_1->p.dict_val_ref = tmp_ele;
    }
    else if (tmp_ele_1->str_type == LIST_HEAD)
    {
        tmp_ele->p.list_next_ref = tmp_ele_1->p.dict_val_ref;
        tmp_ele_1->p.dict_val_ref = tmp_ele;
    }

    return 0;
}



void init_dict(ben_dict_t *dict)
{
    memset(dict, 0, sizeof(ben_dict_t));
}

int ben_coding(char*b, u_32 len,u_32 *pos) {
    char buf[200] = {0};
    int read_len = 0;
    int ret = -1;
    int peek_ret = -1;
    stack_t *s = &g_stack;
    contex_stack_t *c = &g_ctx_stack;
    init_ctx_stack(c);
    init_stack(s);
    init_dict(&dict);

    int n = -1;
    ben_dict_t *d = NULL;
    str_ele_t * tmp_ele = NULL, *tmp_head = NULL;
    str_ele_t * e = NULL;
    char tmp_n_buf[10] ={0};
    int list_head_cnt = 0;
    int dict_head_cnt = 0;

    while ( *pos < len) {
        ret = read_n_char(b,len,pos,1,buf);

        //printf("read a ----- char :(%c), pos (%u)\n", buf[0], *pos);

        if (ret <= 0)
        {
            printf("read failed.\n");
            return -1;
        }

        switch (buf[0])
        {
            case 'd':
                //printf("get a DICT!\n");
                if (buf[0] != 'd' && 1 == *pos)
                {
                    printf("err ,firt letter (%c) should be 'd'\n", buf[0]);
                    return -1;
                }
                ret = push(s, buf[0]);
                if (ret == -1)
                {
                    printf("push failed.\n");
                    return -1;
                }
                status = DICT_WAIT_KEY;

                d = &dict;
                n = d->ele_cnt;
                d->e[n].str_type = DICT;
                sprintf(tmp_n_buf, "Dhead_%d", dict_head_cnt ++);
                memcpy(d->e[n].str, tmp_n_buf, 10 );
                //printf("add to LIST HEAD n=%d\n", n);

                tmp_ele = &d->e[n];
                ctx_push(c,&tmp_ele);
                d->ele_cnt ++;

                break;
            case 'l':
                //printf("get a LIST\n");
                ret = push(s, buf[0]);
                if (ret == -1)
                {
                    printf("push failed.\n");
                    return -1;
                }
                d = &dict;
                n = d->ele_cnt;
                d->e[n].str_type = LIST_HEAD;
                sprintf(tmp_n_buf, "head_%d", list_head_cnt ++);
                memcpy(d->e[n].str, tmp_n_buf, 10 );
                //printf("add to LIST HEAD n=%d\n", n);

                tmp_ele = &d->e[n];
                ctx_push(c,&tmp_ele);
                d->ele_cnt ++;
                break;

            case 'i':
                //printf("get a INT\n");
                if (peek(s) == 'd')
                {
                    if (status == DICT_WAIT_KEY)
                    {
                        //str_type = DICT_KEY;
                        status = DICT_WAIT_VAL;
                    }else if (status == DICT_WAIT_VAL)
                    {
                        //str_type = DICT_VAL;
                        status = DICT_WAIT_KEY;
                    }

                }
                process_a_int(b, len, pos, s,&dict, c);

                break;

            case 'e':
                //printf("end of a (%c)\n", peek(s));
                ret = pop(s);
                if (ret == -1)
                {
                    printf("pop failed.\n");
                    return -1;
                }
                e = NULL;
                tmp_ele = NULL;
                ret = ctx_pop(c, &e);
                if (ret == -1)
                {
                    printf("pop ctx failed.\n");
                    return -1;
                }
                //printf("pop ctx: %s -> %s\n", str_desc[e->str_type], e->str);
                ret = ctx_peek(c, &tmp_ele);
                if (ret == -1)
                {
                    if(c->stack_size == 0 && s->stack_size == 0)
                    {
                        return 0;
                    }
                    printf("peek ctx failed. stack size (%d) (%d)\n", c->stack_size, s->stack_size);
                    return -1;
                }
                //printf("peek ctx: %s -> %s\n", str_desc[tmp_ele->str_type], tmp_ele->str);

                if (tmp_ele->str_type == LIST_HEAD)
                {
                    tmp_head = tmp_ele->p.dict_val_ref;
                    e->p.list_next_ref = tmp_head;
                    tmp_ele->p.dict_val_ref = e;
                } else if(tmp_ele->str_type == DICT_KEY)
                {
                    tmp_ele->p.dict_val_ref = e;
                    //if (status == DICT_WAIT_VAL)
                    if (peek(s) == 'd')
                    {
                        status = DICT_WAIT_KEY;
                    }
                    e = NULL;
                    ctx_pop(c, &e);
                }else if(tmp_ele->str_type == DICT)
                {
                    //if (peek(s) == 'd')
                    //{
                       // status = DICT_WAIT_KEY;
                    //}
                    printf("err here\n");
                }
                break;

            default:
                if (buf[0] > '0' && buf[0] <= '9')
                {
                    *pos -= 1;
                    int str_type = STRING;
                    if (peek(s) == 'd')
                    {
                        if (status == DICT_WAIT_KEY)
                        {
                            str_type = DICT_KEY;
                            status = DICT_WAIT_VAL;
                        }else if (status == DICT_WAIT_VAL)
                        {
                            str_type = DICT_VAL;
                            status = DICT_WAIT_KEY;
                        }

                    }else if (peek(s) == 'l')
                    {
                        str_type = LIST_ELE;
                    }
                    else
                    {
                        printf("WRONG ! peek(s) = %c\n", peek(s));
                    }
                    process_a_string(b, len, pos, s, str_type, &dict, c);
                }
                else if (buf[0] == '0')
                {
                    printf("get a 0 len str!\n");
                    *pos += 1;

                    if (peek(s) == 'd')
                    {
                        if (status == DICT_WAIT_KEY)
                        {
                            status = DICT_WAIT_VAL;
                        }
                        else if (status == DICT_WAIT_VAL)
                        {
                            e = NULL;
                            ctx_pop(c, &e);

                            status = DICT_WAIT_KEY;
                        }

                    }else if (peek(s) == 'l')
                    {

                    }
                    else
                    {
                        printf("WRONG ! peek(s) = %c\n", peek(s));
                    }

                }
                else
                {
                    printf("read a ERR char :(%c)\n", buf[0]);
                    return -1;
                }
                break;
        }
    }

}

void print_result(stack_t *s, contex_stack_t *c)
{
    printf("stack size:%d\n", s->stack_size);
    for (int j = 0; j < s->stack_size; ++j) {
        printf("%c ", s->stack_array[j]);
    }

    printf("--------------------------\n");
    printf("dict size:%d\n", dict.ele_cnt);
    int list_cnt = 0;
    str_ele_t *temp = &dict.e[0];
    /*while (NULL != temp && temp != temp->p.list_next_ref && (temp->str_type >= DICT && temp->str_type <BUTT))
    {
        printf("key: %s\n", temp->str);
        str_ele_t *val = temp->p.dict_val_ref;
        if (NULL != val && (val->str_type >= DICT && val->str_type <BUTT))
        {
            if ( val->str_type == DICT_VAL)
            {
                printf("val: %s\n", val->str);
            }
            val = val->p.list_next_ref;
        }
        temp = temp->p.list_next_ref;
    }
*/

    printf("---------------------------------------\nctx stack size: (%d)\n", c->stack_size);
    for (int p = 0; p < c->stack_size ; ++p)
    {
        //printf("type:[%s],str = (%s)\n",str_desc[c->stack_array[p]->str_type], c->stack_array[p]->str);
    }

    str_ele_t *t = NULL;
    for (int k = 0; k < dict.ele_cnt; ++k)
    {
        switch (dict.e[k].str_type)
        {
        case DICT_KEY:
        //case DICT:
            printf("key: %s-> %s ***** len(%d)\n ",str_desc[dict.e[k].str_type], dict.e[k].str, dict.e[k].str_len);
            if (dict.e[k].p.dict_val_ref != NULL)
            {
                printf("val is: %s, len (%d)\n", dict.e[k].p.dict_val_ref->str, dict.e[k].p.dict_val_ref->str_len);

                str_ele_t *t = dict.e[k].p.dict_val_ref->p.list_next_ref;
                while(t != NULL)
                {
                    //printf("list ele:%s\n", t->str);
                    t = t->p.list_next_ref;
                }
            }

            else
                printf("\n");

            if (dict.e[k].p.list_next_ref != NULL)
                printf("next is: %s\n", dict.e[k].p.list_next_ref->str);
            else
                printf("\n");
            break;
        case DICT_VAL:
            printf("val: %s\n", dict.e[k].str);
            break;
        case LIST_HEAD:
            printf("LIST: %s\n", dict.e[k].str);
            if (dict.e[k].p.list_next_ref != NULL)
                printf("list next is: %s\n", dict.e[k].p.list_next_ref->str);
            else
                printf("\n");

            t = dict.e[k].p.dict_val_ref;
            while(t != NULL)
            {
                //if(t->str_type != LIST_HEAD)
                printf("list ele:%s\n", t->str);
                t = t->p.list_next_ref;
            }
            break;
        case DICT:
            printf("DICT: %s\n", dict.e[k].str);
            if (dict.e[k].p.list_next_ref != NULL)
                printf("dict next is: %s\n", dict.e[k].p.list_next_ref->str);
            else
                printf("\n");

            t = dict.e[k].p.dict_val_ref;
            while(t != NULL)
            {
                //if(t->str_type != LIST_HEAD)
                printf("list ele:%s\n", t->str);
                t = t->p.list_next_ref;
            }
            break;
        default:
            printf("default: %s\n", dict.e[k].str);
            break;
        }

    }

    printf("---------------------------------------\n");

}

long file_size(FILE *fp)
{
    if(!fp)
        return -1;
    fseek(fp,0L,SEEK_END);
    long size=ftell(fp);
    return size;
}
int main22()
{
    //setlocale(LC_ALL, "chs");
//    setlocale(LC_ALL, "zh_CN.UTF-8");
    // char *file_name = "D:\\Download\\ubuntu-14.04.5-desktop-amd64.iso.torrent";
    char *file_name = "E:\\media_spider\\media_spider\\tools\\bencoding\\htpy.torrent";
    //char *file_name = "htpy.torrent";
    //char *file_name = "D:\\Download\\Ů���ֵ�.Women.of.Mafia.2018.1080p.BluRay.x264-��Ӣ˫��-RARBT.torrent";
    FILE *f = fopen(file_name, "rb");
    long torrent_file_size = file_size(f);
    if (torrent_file_size == -1)
    {
        printf("get torrent file size err\n");
        fclose(f);
        return -1;
    }
    if (torrent_file_size > 100*1024)
    {
        printf("too large %ld\n", torrent_file_size);
        fclose(f);
        return -1;
    }
    char *file_buffer = (char*)malloc(torrent_file_size);
    if (NULL == file_buffer)
    {
        printf("alloc mem err\n");
        fclose(f);
        return -1;
    }
    fseek(f,0L,SEEK_SET);

    int r = fread(file_buffer, sizeof(char), (torrent_file_size),f);
    if (r <= 0)
    {
        printf("read err ,size %ld\n", r);
        fclose(f);
        free(file_buffer);
        return -1;
    }

    printf("torrent size: %ld,read size: %d\n", torrent_file_size, r);
    u_32 pos = 0;
    ben_coding(file_buffer, torrent_file_size,&pos);
    print_result(&g_stack, &g_ctx_stack);
    fclose(f);
    free(file_buffer);

    return 0;
}

int main1()
{
    //char *buffer = "d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re";
    //char *buffer = "d2:ip6:10.1.11:rd2:id20:11111111111111111111e1:t2:aa1:y1:re";
    //char *buffer = "d1:eli201e23:A Generic Error Ocurrede1:t2:aa1:y1:ee";
    //char *buffer = "d1:ad2:id20:abcdefghij01234567896:target20:abcdefghij0123456789e1:q9:find_node1:t2:aa1:y1:qe";
    //char *buffer = "d1:rd2:id20:0123456789abcdefghij5:nodes0:e1:t2:aa1:y1:re";
    //char *buffer = "d1:rd2:id20:abcdefghij01234567895:token8:aoeusnth6:valuesl6:axje.u6:idhtnmee1:t2:aa1:y1:re";
    char *buffer = "d1:ad2:id20:abcdefghij01234567896:target20:mnopqrstuvwxyz123456e1:q9:find_node1:t2:aa1:y1:qe";
    u_32 len = strlen(buffer);
    u_32 pos = 0;
    ben_coding(buffer, len, &pos);
    print_result(&g_stack, &g_ctx_stack);
     str_ele_t *temp = &dict.e[0];
     printf("%s\n", temp->str);
     str_ele_t *t = temp->p.dict_val_ref;
     while(t!=NULL)
     {
         printf("%s\n", t->str);
         if (t->str[0] == 'a')
         {
             str_ele_t *t1 = t->p.dict_val_ref->p.dict_val_ref;
             while(NULL != t1)
             {
                 printf("r: [%s]\n", t1->str);
                 if (t1->str[0] == 'i' || t1->str[0] == 't')
                 {
                     str_ele_t *t2 = t1->p.dict_val_ref;
                     while(NULL != t2)
                     {
                         printf("values: [%s]\n", t2->str);
                         t2 = t2->p.list_next_ref;
                     }

                 }
                 t1 = t1->p.list_next_ref;
             }
         }
         t = t->p.list_next_ref;
     }
    return 0;
}
