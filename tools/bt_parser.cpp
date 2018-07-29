//
// Created by xx on 2018/7/27.
//

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <clocale>
#include <afxres.h>
#include <cwchar>
#include "bt_parser.h"

int wait_status = -1;

int push(stack_t *s, char a)
{
    if (s->stack_size >= MAX_STACK_SIZE)
    {
        //printf("err occured push %c\n", a);
        return -1;
    }
    s->stack_array[s->stack_size++] = a;
    return 0;
}

int pop(stack_t *s)
{
    if (s->stack_size <= 0)
    {
        //printf("err occured pop\n");
        return -1;
    }
    s->stack_size --;
    return s->stack_array[s->stack_size];
}

int peek(stack_t *s)
{
    if (s->stack_size <= 0)
    {
        //printf("err occured peek\n");
        return -1;
    }
    return s->stack_array[s->stack_size-1];
}

void init_stack(stack_t *s)
{
    memset(s, 0 , sizeof(stack_t));
}
void read_until_char(char *b, u_32 len, u_32 *pos, char a, char buffer[], int *read_len)
{
    int i = 0;
    //printf("pos untillllll %u\n", *pos);
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
    //printf("len buff %s pos %u i= %d\n", buffer, *pos, i);
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
extern void print_hex(unsigned char*buffer,u_32 n);

void save_announce()
{

}
void save_announce_list()
{

}
void save_comment()
{

}
void process_status_machine(char*string, stack_t *s)
{
    if (wait_status == -1)
    {
        if ( 0 == strcasecmp(string,"announce"))
        {
            wait_status = WAIT_ANN;
        }else if ( 0 == strcasecmp(string,"announce-list"))
        {
            wait_status = WAIT_ANN_LIST;
        }else if ( 0 == strcasecmp(string,"comment"))
        {
            wait_status = WAIT_COMMT;
        }else if ( 0 == strcasecmp(string,"created by"))
        {
            wait_status = WAIT_CREATE_BY;
        }else if ( 0 == strcasecmp(string,"creation date"))
        {
            wait_status = WAIT_CREATE_DATE;
        }else if ( 0 == strcasecmp(string,"encoding"))
        {
            wait_status = WAIT_ENCODING;
        }else if ( 0 == strcasecmp(string,"info"))
        {
            wait_status = WAIT_INFO_CONTENT;
        }else if ( 0 == strcasecmp(string,"files"))
        {
           // wait_status = WAIT_INFO;
        }

    } else if(wait_status == WAIT_ANN)
    {
        save_announce();
        wait_status = -1;
    } else if(wait_status == WAIT_ANN_LIST)
    {
        int ret = peek(s);
        if (ret == -1)
        {
            printf("err \n");
            return ;
        }
        if ('l' == ret )
        {
            save_announce_list();
        } else
        {
            wait_status = -1;
        }

        //wait_status = -1;
    } else if(wait_status == WAIT_COMMT)
    {
        save_comment();
        wait_status = -1;
    }else if(wait_status == WAIT_CREATE_BY)
    {
        save_comment();
        wait_status = -1;
    }else if(wait_status == WAIT_CREATE_DATE)
    {
        save_comment();
        wait_status = -1;
    }else if(wait_status == WAIT_ENCODING)
    {
        save_comment();
        wait_status = -1;
    }
    else if(wait_status == WAIT_INFO_CONTENT)
    {
        if ( 0 == strcasecmp(string,"files"))//multi files
        {
            wait_status = WAIT_FILES_LEN_STR;
        } else  if ( 0 == strcasecmp(string,"length")) //single file
        {

        }
    } else if(wait_status == WAIT_FILES_LEN_STR)
    {
        if ( 0 == strcasecmp(string,"length"))
        {
            wait_status == WAIT_FILES_LEN_VALUE;
        }
    } else if(wait_status == WAIT_FILES_LEN_VALUE)
    {
        //save file len
        wait_status = WAIT_FILES_PATH_STR;
    } else if(wait_status == WAIT_FILES_PATH_STR)
    {
        if ( 0 == strcasecmp(string,"path"))
        {
            wait_status == WAIT_FILES_PATH_VALUE;
        }
    }else if(wait_status == WAIT_FILES_PATH_VALUE)
    {
        //save file path
        wait_status == WAIT_FILES_PATH_VALUE;
    }
}

void process_by_ele_status(char *val, stack_t*e, stack_t*s)
{
    int peek_ret = peek(e);
    if (-1 == peek_ret)
    {
        printf("err occured\n");
        return;
    }
    size_t len_ = strlen(val);
    switch (peek_ret)
    {
        case ELE_KEY:
            printf("get ann key:%s\n",val);
            pop(e);
            push(e,ELE_VALUE);
            break;
        case ELE_VALUE:
            //save value

            if (len_ < 200)
            {
                printf("get an val:%s\n", val);
            }
            else
            {
                print_hex((unsigned char*)val, len_);
            }

            save_announce();
            pop(e);
            break;
        case ELE_LIST:
            printf("get a list:%s\n", val);
            save_announce_list();
            break;
        case ELE_ANN_COMMT:
            save_comment();
            pop(e);
            break;
    }

}

int process_a_string(char*b,u_32 len,u_32 *pos,stack_t *e,stack_t*s)
{
    int ret = -1;
    int read_len = -1;
    char buf[200] = {0};
    char *tmp_big_buf = NULL;
    read_until_char(b, len, pos,':',buf,&read_len);
    if (read_len <= 0)
    {
        printf("read ':' failed.\n");
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
        //print_hex((unsigned char*)tmp_big_buf, 20);

    //printf("string-> %s\n", tmp_key);

    int peek_ret = peek(e);

    if(-1 == peek_ret || (ELE_VALUE != peek_ret) && (ELE_LIST != peek_ret))
    {
        peek_ret = peek(s);
        if (-1 == peek_ret)
        {
            printf("errrr\n");
            return -1;
        }

        if ('d' == peek_ret)
        {
            ret = push(e, ELE_KEY);
            if (ret == -1) {
                printf("push failed.\n");
                return -1;
            }
        }else if ('l' == peek_ret)
        {
            ret = push(e, ELE_LIST);
            if (ret == -1) {
                printf("push failed.\n");
                return -1;
            }
        }
        else if ('i' == peek_ret)
        {
            ret = push(e, ELE_INT);
            if (ret == -1) {
                printf("push failed.\n");
                return -1;
            }
        }
        ;
    }

    process_by_ele_status(tmp_big_buf, e, s);

    free(tmp_big_buf);
    return 0;
}

int process_a_int(char *b,u_32 len, u_32*pos, stack_t*e,stack_t*s)
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
    //printf("integer -> %s\n", buf);

    if (e->stack_size == 0)
    {
        int peek_ret = peek(s);
        if (-1 == peek_ret)
        {
            printf("errrr\n");
            return -1;
        }

        if ('d' == peek_ret)
        {
            ret = push(e, ELE_KEY);
            if (ret == -1) {
                printf("push failed.\n");
                return -1;
            }
        }else if ('l' == peek_ret)
        {
            ret = push(e, ELE_LIST);
            if (ret == -1) {
                printf("push failed.\n");
                return -1;
            }
        }
        else if ('i' == peek_ret)
        {
            ret = push(e, ELE_INT);
            if (ret == -1) {
                printf("push failed.\n");
                return -1;
            }
        }
    }


    process_by_ele_status(buf, e, s);

    return 0;
}

stack_t g_stack;
stack_t ele_stack;//for element recognize
int ben_coding(char*b, u_32 len,u_32 *pos) {
    char buf[200] = {0};
    int read_len = 0;
    int ret = -1;
    int peek_ret = -1;
    stack_t *s = &g_stack;
    stack_t *e = &ele_stack;
    init_stack(s);
    init_stack(e);

    while ( *pos < len) {
        peek_ret = peek(s);
        if (-1 == peek_ret) {
            ret = read_n_char(b,len,pos,1,buf);
            if (ret <= 0) {
                printf("read failed.\n");
                return -1;
            }
            ret = push(s, buf[0]);
            if (ret == -1) {
                printf("push failed.\n");
                return -1;
            }
            //buf[0] must be 'd'
//            ret = process_a_key_value(f);
            //printf("pos %u\n", *pos);
            ret = process_a_string(b,len,pos,e,s);
            if (-1 == ret) {
                return -1;
            }
        } else {
            ret = read_n_char(b,len,pos,1,buf);
            if (ret <= 0) {
                printf("read failed.\n");
                return -1;
            }

            //printf("cur ret : %c\n", buf[0]);
            //printf("cur peek : %c\n", peek_ret);
            switch (peek_ret) {
                case 'd':
                case 'l':
                    if (buf[0] == 'e') {

                        ret = pop(s);
                        if (ret == -1) {
                            return -1;
                        }
                        int e_ret = pop(e);
                        if (e_ret == -1) {
                            return -1;
                        }
                        // follow code because exists this case - - - d key = xxx,value = list, when lists in value pop
                        // over, ELE_VALUE(pushed to s by key) should be pop too!
                        if ('d' == peek(s) && peek(e) == ELE_VALUE)
                        {
                            e_ret = pop(e);
                            if (e_ret == -1) {
                                return -1;
                            }
                        }
                        //printf("pop ele %c peek:%c\n", e_ret, peek(e));
                        //printf("pop one char %c \n", ret);
                    } else if (buf[0] == 'l' || buf[0] == 'd') {
                        //printf("push one char : %c\n", buf[0]);
                        ret = push(s, buf[0]);
                        if (ret == -1) {
                            return -1;
                        }
                        if(buf[0] == 'd')
                        {
                            ret = push(e, ELE_KEY);
                            if (ret == -1) {
                                return -1;
                            }
                        }
                        if(buf[0] == 'l')
                        {
                            ret = push(e, ELE_LIST);
                            if (ret == -1) {
                                return -1;
                            }
                        }

                    } else if (buf[0] == 'i') {
                        ret = process_a_int(b,len,pos,e,s);
                        if (ret == -1) {
                            return -1;
                        }
                    } else {
                        *pos -= 1;
                        process_a_string(b,len,pos,e,s);
                    }
                    break;

            }
        }
    }
    printf("stack size:%d\n", s->stack_size);
    for (int j = 0; j < s->stack_size; ++j) {
        printf("%c ", s->stack_array[j]);
    }

}
long file_size(FILE *fp)
{
    if(!fp)
        return -1;
    fseek(fp,0L,SEEK_END);
    long size=ftell(fp);

    return size;
}
int main()
{
    //setlocale(LC_ALL, "chs");
//    setlocale(LC_ALL, "zh_CN.UTF-8");
   // char *file_name = "D:\\Download\\ubuntu-14.04.5-desktop-amd64.iso.torrent";
    char *file_name = "D:\\Download\\.4354421E5D9274510A9903445A7A2D42C10C8B34.torrent";
//    char *file_name = "D:\\Download\\女黑手党.Women.of.Mafia.2018.1080p.BluRay.x264-中英双字-RARBT.torrent";
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
    fclose(f);
    free(file_buffer);
    file_buffer = NULL;

//    FILE *f1 = fopen(file_name, "rb");
//    char type[2] = {0};
//    read_n_char(f1, 2, type);
//    print_hex((unsigned char*)type, 2);
//    printf("%c %c" , type[0], type[1]);
//    fclose(f1);
    return 0;
}
