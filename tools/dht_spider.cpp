//
// Created by xx on 2018/8/26.
//

#include <cstdio>
#include "dht_spider.h"
#include "bt_parser.h"

extern stack_t g_stack;
extern contex_stack_t g_ctx_stack;
extern ben_dict_t dict;
//extern int ben_coding(char*b, u_32 len,u_32 *pos);

extern void print_hex(unsigned char* n, u_32 len);
extern void sha_1(u_char *s,u_64 total_len,u_32 *hh);
extern int ben_coding(char*b, u_32 len,u_32 *pos);
extern void print_result(stack_t *s, contex_stack_t *c);

int cmp_node_str(unsigned char * str1, unsigned char *str2)
{
    for (int i = NODE_STR_LEN -1; i >= 0; --i)
    {
        if (str1[i] >= str2[i])
        {
            return 1;
        } else if(str1[i] < str2[i])
        {
            return -1;
        }
    }
    return 0;
}

/**  begin <= node_str < end */
bool is_node_str_in_range(unsigned char *node_str, int begin, int end)
{
    unsigned char tmp_b[NODE_STR_LEN+1] = {0};
    unsigned char tmp_e[NODE_STR_LEN+1] = {0};
    int begin_idx = begin / 8;
    int end_idx = end / 8;
    unsigned char b = (unsigned char)(1 << (begin % 8));
    unsigned char e = (unsigned char)(1 << (end % 8));
    tmp_b[begin_idx] |= b;
    tmp_e[end_idx] |= e;

    int ret_bigger = cmp_node_str(node_str, tmp_b);
    int ret_less = cmp_node_str(node_str, tmp_e);
    if (end == 160)
    {
        ret_less = 1;
    }

    if (1 == ret_bigger && -1 == ret_less)
    {
        return true;
    }

    return false;
}

bool bucket_contain_node(bucket_t* bucket, unsigned char* node_str)
{
    for (int i = 0; i < BUCKET_SIZE; ++i)
    {
        if (0 == memcmp((void*)node_str,(void*)bucket->neibor_nodes[i],NODE_STR_LEN))
        {
            return true;
        }
    }
    return false;
}

void update_bucket_update_time(bucket_t*bucket)
{
    time_t t ;
    time(&t);
    bucket->update_time = t;
}
/** cur code maybe cause stack overflow,optimaze later */
int insert_into_bucket(unsigned char *node_str, bucket_t *bucket, node_t *node)
{
    if (bucket->nodes_num >= BUCKET_SIZE)
    {
        //split into two buckets
        int low_start = bucket->range_start;
        int low_end = bucket->range_end - 1;
        int hi_start = bucket->range_end - 1;
        int hi_end = bucket->range_end;

        bucket_t tmp_low = {0};
        bucket_t tmp_hi = {0};

        tmp_low.range_start = low_start;
        tmp_low.range_end = low_end;
        tmp_low.nodes_num = 0;

        tmp_hi.range_start = hi_start;
        tmp_hi.range_end = hi_end;
        tmp_hi.nodes_num = 0;

        for (int i = 0; i < BUCKET_SIZE; ++i)
        {
            if (is_node_str_in_range(bucket->neibor_nodes[i],low_start, low_end))
            {
                insert_into_bucket(bucket->neibor_nodes[i], &tmp_low,node);
            } else if (is_node_str_in_range(bucket->neibor_nodes[i], hi_start, hi_end))
            {
                insert_into_bucket(bucket->neibor_nodes[i], &tmp_hi, node);
            } else{
                printf("there must be some err\n");
                print_hex(bucket->neibor_nodes[i],NODE_STR_LEN);
                printf("low: %d->%d, hi:%d->%d\n", low_start, low_end, hi_start, hi_end);
            }
        }

        if (is_node_str_in_range(node_str,low_start, low_end))
        {
            insert_into_bucket(node_str, &tmp_low, node);
            update_bucket_update_time(&tmp_low);
        } else if (is_node_str_in_range(node_str, hi_start, hi_end))
        {
            insert_into_bucket(node_str, &tmp_hi, node);
            update_bucket_update_time(&tmp_hi);
        } else{
            printf("there must be some errs\n");
            print_hex(node_str,NODE_STR_LEN);
            printf("low: %d->%d, hi:%d->%d\n", low_start, low_end, hi_start, hi_end);
        }
        // cpy tmp low & tmp hi to node buckets
        memcpy(bucket, &tmp_low, sizeof(bucket_t));
        //node->buckets_num ++;
        memcpy(&node->buckets[node->buckets_num], &tmp_hi, sizeof(bucket_t));
        node->buckets_num ++;

    }
    else if(false == bucket_contain_node(bucket, node_str))
    {
        memcpy(bucket->neibor_nodes[bucket->nodes_num], node_str, NODE_STR_LEN);
        bucket->status = IN_USE;
        bucket->nodes_num ++;
        update_bucket_update_time(bucket);
    }
    return OK;
}
int ping_node(SOCKET s, sockaddr_in* addr, unsigned char* self_node_id)
{
    //d1:ad2:id20:abcdefghij0123456789e1:q4:ping1:t2:aa1:y1:qe
    unsigned char ping_str_start[100] = "d1:ad2:id20:";
    unsigned char ping_str_end[100] = "e1:q4:ping1:t2:aa1:y1:qe";
    unsigned char ping_str_self_node_id[100] = "";
    memcpy(ping_str_self_node_id, self_node_id, NODE_STR_LEN);
    strcat((char*)ping_str_start, (char*)ping_str_self_node_id);
    strcat((char*)ping_str_start, (char*)ping_str_end);
    sendto(s,(char *)ping_str_start, strlen((char*)ping_str_start), 0,(sockaddr*)addr, sizeof(struct sockaddr));
    printf("send ping msg: %s\n", ping_str_start);
    return OK;
}

int query_node()
{
    return OK;
}
int find_node()
{
    return OK;
}

int announce_node()
{
    return OK;
}
/** rsp other node query */
int on_ping_node()
{
    return OK;
}

int on_query_node()
{
    return OK;
}
int on_find_node()
{
    return OK;
}

int on_announce_node()
{
    return OK;
}

int get_ip_in_ping_rsp(ben_dict_t *dict, u_32 *ip, u_16 *port)
{
    str_ele_t *e = dict->e[0].p.dict_val_ref;
    while (NULL != e)
    {
        if (0 == strcmp(e->str, "ip"))
        {
            printf("\nget ping rsp ip: %s\n", e->p.dict_val_ref->str);
            memcpy(ip, e->p.dict_val_ref->str, sizeof(u_32));
            memcpy(port,e->p.dict_val_ref->str + sizeof(u_32), sizeof(u_16));
            return 0;
        }
        e = e->p.list_next_ref;
    }
    printf("\n not find ip value!\n");
    return -1;
}

int rcv_msg(SOCKET s)
{
    struct sockaddr_in in_add;
    int in_add_len = sizeof(struct sockaddr_in);
    int buffer_len = 500;
    char buffer[500] = {0};
    int ret = recvfrom(s,buffer,buffer_len,0,(sockaddr*)&in_add,&in_add_len);
    if (ret > 0)
    {
        printf("rcv msg:%s\n", buffer);
        u_32 len = strlen(buffer);
        u_32 pos = 0;
        ben_coding(buffer, len, &pos);
        return OK;
    } else{
        //char *buffer = "d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re";
    //char *buffer = "d2:ip6:10.1.11:rd2:id20:11111111111111111111e1:t2:aa1:y1:re";
        //char b[500] = "d1:eli201e23:A Generic Error Ocurrede1:t2:aa1:y1:ee";
        char b[500]= "d2:ip6:10.1.11:rd2:id20:11111111111111111111e1:t2:aa1:y1:re";
        printf("rcv msg:%s\n", b);
        u_32 len = strlen(b);
        u_32 pos = 0;
        ben_coding(b, len, &pos);
        print_result(&g_stack, &g_ctx_stack);
        u_32 ip = 0;
        u_16 port = 0;
        get_ip_in_ping_rsp(&dict, &ip, &port);
        printf("\nbefore covert :%u\n-> %x\n", ip, ip);
        u_32 _ip = ntohl(ip);
        printf("\nafter covert :%u\n-> %x\n", _ip, _ip);

        return OK;

        printf("no rcv msg\n");
        return ERR;
    }
}

void concat_hex(u_32 *hash, u_32 len,u_char *ret_str)
{
    u_32 char_len = sizeof(u_32) * len;
    unsigned char *tmp = (unsigned char*)hash;
    for (u_32 i = 0; i <  char_len; ++i)
    {
        sprintf((char*)(ret_str+i), "%02x", (tmp+i));
    }
}

int random_node_id(unsigned char *node_id)
{
    time_t t ;
    time(&t);
    char time_str[100] = {0};
    printf("now:%u\n", t);
    sprintf(time_str,"%u", t);
    u_32 _hash [5] = {0};
    sha_1((u_char*)time_str,strlen(time_str), _hash);
    print_hex((u_char*)_hash,20);
    memcpy(node_id, "abcdefghij0123456789", NODE_STR_LEN);
    return OK;
}

int init_self_node_id(node_t*node)
{
    return random_node_id(node->node_id);
}

int init_wsa()
{
    WORD sock_ver = MAKEWORD(2,2);
    WSADATA wsadata ;
    if (WSAStartup(sock_ver,&wsadata))
    {
        printf("err no:%d\n",errno);
        return -1;
    }
    return 0;
}

int init_rcv_socket(node_t *node)
{
    node->recv_socket = INVALID_SOCKET;
    node->recv_socket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if (INVALID_SOCKET == node->recv_socket)
    {
        printf("new socket err");
        return -1;
    }
    u_long mode = 1;//set none-block socket
    int r_ctl = ioctlsocket(node->recv_socket,FIONBIO,&mode);
    if (r_ctl != 0)
    {
        printf("ioctl err:%d\n", r_ctl);
        closesocket(node->recv_socket);
        return -1;
    }
    sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(6881);
    local_addr.sin_addr.S_un.S_addr = INADDR_ANY;

    int ret = bind(node->recv_socket,(struct sockaddr*)&local_addr, sizeof(struct sockaddr));
    if (0 != ret)
    {
        printf("bind err:%d\n", ret);
        closesocket(node->recv_socket);
        return -1;
    }

    return 0;
}

int init_send_socket(node_t *node)
{
    node->send_socket = INVALID_SOCKET;
    node->send_socket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if (INVALID_SOCKET == node->send_socket)
    {
        printf("new socket err");
        closesocket(node->send_socket);
        return -1;
    }

    u_long mode = 1;//set none-block socket
    int r_ctl = ioctlsocket(node->send_socket,FIONBIO,&mode);
    if (r_ctl != 0)
    {
        printf("ioctl err2:%d\n", r_ctl);
        closesocket(node->send_socket);
        return -1;
    }

    return 0;
}

void clear_node(node_t *node)
{
    WSACleanup();
    closesocket(node->send_socket);
    closesocket(node->recv_socket);
}

int init_self_node(node_t *node)
{
    int ret = -1;
    memset(node,0, sizeof(node_t));
    ret = init_wsa();
    if (-1 == ret)
    {
        return -1;
    }
    ret = init_rcv_socket(node);
    if (-1 == ret)
    {
        return -1;
    }
    ret = init_send_socket(node);
    if (-1 == ret)
    {
        return -1;
    }
    ret = init_self_node_id(node);
    return 0;
}

int init_route_table(node_t*node)
{
    //ping bootstrap node
    sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(6881);
    remote_addr.sin_addr.S_un.S_addr = (inet_addr("67.215.246.10"));
     int ret = 0;//ping_node(node->send_socket,&remote_addr,node->node_id);
     int try_times = 50;
     while (try_times -- > 0)
     {
         printf("rcv msg times %d:\n", 50 - try_times);
         ret = rcv_msg(node->send_socket);
         if (ret == OK)
         {
             break;
         }
         Sleep(1);
     }

    /**
     * add ret to except node list
     *
     * */
     return ret;
}

int worker(node_t* node)
{

    while(true)
    {
        // if
    }
}

int main()
{
    printf("ok dht spider\n");
    node_t node = {0};
    int ret = init_self_node(&node);
    if (-1 == ret)
    {
        clear_node(&node);
        return -1;
    }
    init_route_table(&node);
    //worker(&node);

    clear_node(&node);

    return 0;
}
