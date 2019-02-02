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

int insert_into_bucket_tree(node_t *node, bucket_tree_node_t **root,u_8 *node_str, u_8 *node_ip, u_8 *node_port, u_8 *self_node_id);
int save_route_tbl_to_file(node_t* node);

u_8 tid = 0;

u_16 dht_ntohs(u_16 x)
{
    u_8 *low = (u_8*)&x;
    u_8 *high = (low + 1);
    u_8 tmp = *low;
    *low = *high;
    *high = tmp;
    return x;
}

int tree_push(tree_stack_t *s, bucket_tree_node_t** a)
{
    if (s->stack_size >= MAX_STACK_SIZE)
    {
        return -1;
    }
    s->stack_array[s->stack_size++] = *a;
    return 0;
}

int tree_pop(tree_stack_t *s, bucket_tree_node_t **a)
{
    if (s->stack_size <= 0)
    {
        return -1;
    }
    s->stack_size --;
    *a = s->stack_array[s->stack_size];
    return 0;
}

int tree_peek(tree_stack_t *s, bucket_tree_node_t **a)
{
    if (s->stack_size <= 0)
    {
        return -1;
    }
    *a = s->stack_array[s->stack_size-1];
    return 0;
}

bool tree_stack_is_empty(tree_stack_t *s)
{
    return (s->stack_size <= 0);
}

void init_tree_stack(tree_stack_t *s)
{
    memset(s, 0 , sizeof(tree_stack_t));
}

/**
* cmp str len is NODE_STR_LEN+1
*/
int cmp_node_str(unsigned char * str1, unsigned char *str2)
{
    for (int i = 0; i <= NODE_STR_LEN; ++i)
    {
        if (str1[i] > str2[i])
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

int ping_node(SOCKET s, sockaddr_in* addr, unsigned char* self_node_id)
{
    //d1:ad2:id20:abcdefghij0123456789e1:q4:ping1:t2:aa1:y1:qe
    unsigned char ping_str_start[100] = "d1:ad2:id20:";
    unsigned char ping_str_end[100] = "e1:q4:ping1:t2:pp1:y1:qe";
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
/**
* find node :the t value,start with f.
*/
int find_node(SOCKET s, sockaddr_in* addr, u_8 * id, u_8 *tgt)
{
    //d1:ad2:id20:abcdefghij01234567896:target20:mnopqrstuvwxyz123456e1:q9:find_node1:t2:aa1:y1:qe
    char str1[] = "d1:ad2:id20:";
    char str2[] = "6:target20:";
    char str3[] = "e1:q9:find_node1:t2:f";
    tid +=1;
    char str4[] = "1:y1:qe";

    char snd_str[200] = {0};
    char*send_str = &snd_str[0];
    //save context and send
    memcpy(send_str, (void*)(const char*)str1, strlen(str1));
    send_str += strlen(str1);

    memcpy(send_str, id, NODE_STR_LEN);
    send_str += NODE_STR_LEN;

    memcpy(send_str, str2, strlen(str2));
    send_str += strlen(str2);

    memcpy(send_str, tgt, NODE_STR_LEN);
    send_str += NODE_STR_LEN;

    memcpy(send_str, str3, strlen(str3));
    send_str += strlen(str3);

    *send_str ='d';// tid ++;
    send_str +=1;

    memcpy(send_str, str4, strlen(str4));

    int send_len = strlen("d1:ad2:id20:abcdefghij01234567896:target20:mnopqrstuvwxyz123456e1:q9:find_node1:t2:aa1:y1:qe");

    sendto(s,(char *)snd_str, send_len, 0,(sockaddr*)addr, sizeof(struct sockaddr));
    printf("send find node msg(len:%d): %s\n", send_len, snd_str);

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

//peer node response my requst
int get_peer_rsp_r_type(ben_dict_t *dict, int *type, u_16 *t)
{
    str_ele_t *e = dict->e[0].p.dict_val_ref;
    while (NULL != e)
    {
        if (0 == strcmp(e->str, "t"))
        {
            printf("\nget rsp t: %s\n", e->p.dict_val_ref->str);
            char type_str = e->p.dict_val_ref->str[0];
            switch (type_str)
            {
            case 'p'://ping
                *type = Y_TYPE_PING_RSP;
                return 0;
            case 'f'://find_node
                *type = Y_TYPE_FIND_NODE_RSP;
                return 0;
            case 'a'://announce peer
                *type = Y_TYPE_ANNOUNCE_PEER_RSP;
                return 0;
            case 'g'://get peers
                *type = Y_TYPE_GET_PEER_RSP;
                return 0;
            default:

                break;

            }
            //return 0;
        }
        e = e->p.list_next_ref;
    }
    return -1;
}
/**
* peer request to me.
*/
int get_peer_req_q_type(ben_dict_t *dict, int *type)
{
    str_ele_t *e = dict->e[0].p.dict_val_ref;
    while (NULL != e)
    {
        if (0 == strcmp(e->str, "q"))
        {
            printf("\nget rsp q: %s\n", e->p.dict_val_ref->str);
            char type_str = e->p.dict_val_ref->str[0];
            switch (type_str)
            {
            case 'p'://ping
                *type = Y_TYPE_PING;
                return 0;
            case 'f'://find_node
                *type = Y_TYPE_FIND_NODE;
                return 0;
            case 'a'://announce peer
                *type = Y_TYPE_ANNOUNCE_PEER;
                return 0;
            case 'g'://get peers
                *type = Y_TYPE_GET_PEER;
                return 0;
            default:

                break;

            }
        }
        e = e->p.list_next_ref;
    }
    return -1;
}


int get_rsp_msg_type(ben_dict_t *dict, int *rsp_msg_type)
{
    /**
    * pingÇëÇó={"t":"aa", "y":"q","q":"ping", "a":{"id":"abcdefghij0123456789"}}

B±àÂë=d1:ad2:id20:abcdefghij0123456789e1:q4:ping1:t2:aa1:y1:qe

»Ø¸´={"t":"aa", "y":"r", "r":{"id":"mnopqrstuvwxyz123456"}}

B±àÂë=d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re


find_nodeÇëÇó={"t":"aa", "y":"q","q":"find_node", "a":{"id":"abcdefghij0123456789","target":"mnopqrstuvwxyz123456"}}

B±àÂë=d1:ad2:id20:abcdefghij01234567896:target20:mnopqrstuvwxyz123456e1:q9:find_node1:t2:aa1:y1:qe

»Ø¸´={"t":"aa", "y":"r", "r":{"id":"0123456789abcdefghij", "nodes":"def456..."}}

B±àÂë=d1:rd2:id20:0123456789abcdefghij5:nodes9:def456...e1:t2:aa1:y1:re


get_peersÇëÇó={"t":"aa", "y":"q","q":"get_peers", "a":{"id":"abcdefghij0123456789","info_hash":"mnopqrstuvwxyz123456"}}

B±àÂë=d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz123456e1:q9:get_peers1:t2:aa1:y1:qe

»Ø¸´peers ={"t":"aa", "y":"r", "r":{"id":"abcdefghij0123456789", "token":"aoeusnth","values": ["axje.u", "idhtnm"]}}

B±àÂë=d1:rd2:id20:abcdefghij01234567895:token8:aoeusnth6:valuesl6:axje.u6:idhtnmee1:t2:aa1:y1:re

»Ø¸´×î½Ó½üµÄnodes= {"t":"aa", "y":"r", "r":{"id":"abcdefghij0123456789", "token":"aoeusnth","nodes": "def456..."}}

B±àÂë=d1:rd2:id20:abcdefghij01234567895:nodes9:def456...5:token8:aoeusnthe1:t2:aa1:y1:re


announce_peersÇëÇó={"t":"aa", "y":"q","q":"announce_peer", "a":{"id":"abcdefghij0123456789","info_hash":"mnopqrstuvwxyz123456", "port":6881, "token": "aoeusnth"}}

B±àÂë=d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz1234564:porti6881e5:token8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe

»Ø¸´={"t":"aa", "y":"r", "r":{"id":"mnopqrstuvwxyz123456"}}

B±àÂë=d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re


    */
    u_16 t = 0;
    str_ele_t *e = dict->e[0].p.dict_val_ref;
    while (NULL != e)
    {
        if (0 == strcmp(e->str, "y"))
        {
            printf("\nget rsp y: %s\n", e->p.dict_val_ref->str);
            char type_str = e->p.dict_val_ref->str[0];
            switch (type_str)
            {
            case 'q'://req
                //*rsp_msg_type = Y_TYPE_Q;
                return get_peer_req_q_type(dict, rsp_msg_type);
            case 'r'://rsp
                *rsp_msg_type = Y_TYPE_R;

                return get_peer_rsp_r_type(dict, rsp_msg_type,&t);
            case 'e'://rsp err
                *rsp_msg_type = Y_TYPE_E;
                return 0;
            default:

                break;

            }
        }
        e = e->p.list_next_ref;
    }
    printf("\n not find y value!\n");
    return -1;


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

int get_id_in_ping_rsp(ben_dict_t *dict, u_8 *id)
{
    printf("\nwtf\n");
    str_ele_t *e = dict->e[0].p.dict_val_ref;
    while (NULL != e)
    {
        if (0 == strcmp(e->str, "r"))
        {
            printf("\nget ping rsp r: %s\n", e->p.dict_val_ref->str);
            str_ele_t *e1 = e->p.dict_val_ref->p.dict_val_ref;
            while (NULL != e1)
            {
                if (0 == strcmp(e1->str, "id"))
                {
                    printf("\nget ping rsp id of r: %s\n", e1->p.dict_val_ref->str);
                    memcpy(id, e1->p.dict_val_ref->str, 20*sizeof(u_8));
                    return 0;
                }
            }

        }
        e = e->p.list_next_ref;
    }
    printf("\n not find id value!\n");
    return -1;
}

void inet_bin_to_string(u_32 ip, char output[])
{
    unsigned char *tmp = (unsigned char*)&ip;
    char buf[4] = {0};
    for (int i = 0; i < 4; ++i)
    {
        memset(buf, 0, 4);
        sprintf(buf, "%hhu", tmp[i]);
        strcat(output, buf);
        if (i != 3)
            strcat(output, ".");
    }
}

void handle_ping_rsp()
{

}

int handle_find_node_rsp(ben_dict_t *dict, node_t *node)
{
    u_8 id[NODE_STR_LEN] = {0};
    u_8 nodes_str[500] = {0};
    int nodes_str_len = 0;
    bool find_node_str = false;
    str_ele_t *e = dict->e[0].p.dict_val_ref;
    while (NULL != e)
    {
        if (0 == strcmp(e->str, "r"))
        {
            printf("\nget find node rsp r: %s\n", e->p.dict_val_ref->str);
            str_ele_t *e1 = e->p.dict_val_ref->p.dict_val_ref;
            while (NULL != e1)
            {
                if (0 == strcmp(e1->str, "id"))
                {
                    printf("\nget find node rsp id of r: %s\n", e1->p.dict_val_ref->str);
                    memcpy(id, e1->p.dict_val_ref->str, 20*sizeof(u_8));
                }
                if (0 == strcmp(e1->str, "nodes"))
                {
                    nodes_str_len = e1->p.dict_val_ref->str_len;
                    printf("\nget ping rsp nodes of r(len:%d): %s\n", nodes_str_len, e1->p.dict_val_ref->str);
                    memcpy(nodes_str, e1->p.dict_val_ref->str, nodes_str_len*sizeof(u_8));
                    //return 0;
                    find_node_str = true;
                }

                e1 = e1->p.list_next_ref;
            }

        }
        e = e->p.list_next_ref;
    }
    if (false == find_node_str)
    {
       printf("\n not find key r !\n");
        return -1;
    }
    int node_ip_port_len = (NODE_STR_LEN + NODE_STR_IP_LEN + NODE_STR_PORT_LEN);
    int node_num = nodes_str_len / node_ip_port_len;
    printf("nodes_len (%d), num (%d)\n", nodes_str_len, node_num);
    if (node_num *node_ip_port_len != nodes_str_len )
    {
        printf("nodes string is not compelete..\n");
        return -1;
    }

    u_8 * temp =(u_8*)nodes_str;
    int ret = -1;
    for (int j = 0; j < node_num ; ++j )
    {
        for(int k=0;k< 26; ++k)
        {
            printf("%hhu.", *(temp+j*26+k) );
        }
        printf("\n");
    }
    for (int i= 0; i < node_num; ++i)
    {
        u_8 *node_str = temp;
        u_8 *node_ip = temp + NODE_STR_LEN;
        u_8 *node_port = temp + NODE_STR_LEN + NODE_STR_IP_LEN;
        ret = insert_into_bucket_tree(node, &node->bkt_tree, node_str, node_ip, node_port, node->node_id);
        if (-1 == ret)
        {
            return -1;
        }

        temp += (NODE_STR_LEN+ NODE_STR_IP_LEN + NODE_STR_PORT_LEN);
    }
    printf("insert one record to memory!\n now save to file\n");
    save_route_tbl_to_file(node);
    return 0;

}
int rcv_msg(SOCKET s, node_t*node)
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
        ben_coding(buffer, ret, &pos);

        print_result(&g_stack, &g_ctx_stack);
        int rsp_type = -1;
        //get_rsp_type(&dict, &rsp_type);
        if (-1 == rsp_type)
        {
            printf("get rsp type failed.\n");
            return OK;
        }
        switch(rsp_type)
        {
        case Y_TYPE_PING_RSP:
            handle_ping_rsp();

            break;
        case Y_TYPE_FIND_NODE_RSP:
            handle_find_node_rsp(&dict, node);
            break;

        default:
            break;

        }
        /*
        u_32 ip = 0;
        u_16 port = 0;
        get_ip_in_ping_rsp(&dict, &ip, &port);

        u_8 id[21] ={0};
        get_id_in_ping_rsp(&dict, id);

        printf("\nbefore covert ip:%u-> %x\n port: %hu -> %x\n", ip, ip, port, port);
        u_32 _ip = ntohl(ip);
        u_16 _port = ntohs(port);
        printf("\nafter covert ip:%u-> %x\nport: %hu -> %x\n", _ip, _ip, _port, _port);

        char dst[16] = {0};
        inet_bin_to_string(_ip,dst);
        printf("\n the ip presentation is %s\n", dst);

        */
        return OK;

    } else{
        //char *buffer = "d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re";
    //char *buffer = "d2:ip6:10.1.11:rd2:id20:11111111111111111111e1:t2:aa1:y1:re";
        //char b[500] = "d1:eli201e23:A Generic Error Ocurrede1:t2:aa1:y1:ee";
        /*char b[500]= "d2:ip6:10.1.11:rd2:id20:11111111111111111111e1:t2:aa1:y1:re";
        printf("rcv msg:%s\n", b);
        u_32 len = strlen(b);
        u_32 pos = 0;
        ben_coding(b, len, &pos);
        print_result(&g_stack, &g_ctx_stack);
        u_32 ip = 0;
        u_16 port = 0;
        get_ip_in_ping_rsp(&dict, &ip, &port);

        u_8 id[21] ={0};
        get_id_in_ping_rsp(&dict, id);

        printf("\nbefore covert ip:%u-> %x\n port: %hu -> %x\n", ip, ip, port, port);
        u_32 _ip = ntohl(ip);
        u_16 _port = ntohs(port);
        printf("\nafter covert ip:%u-> %x\nport: %hu -> %x\n", _ip, _ip, _port, _port);

        char dst[16] = {0};
        inet_bin_to_string(_ip,dst);
        printf("\n the ip presentation is %s\n", dst);

        return OK;*/

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
    printf("random self id hash:");
    print_hex((u_char*)_hash,20);
    //memcpy(node_id, "abcdefghij0123456780", NODE_STR_LEN);
    memcpy(node_id, _hash, NODE_STR_LEN);
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

void convert_node_str(u_8 *src, u_8 * dst)
{
    memcpy(dst+1, src, NODE_STR_LEN);
}

bool is_node_leaf(bucket_tree_node_t*bkt_tree)
{
    return (NULL == bkt_tree->l && NULL == bkt_tree->r);
}

/**
* here node_str is NODE_STR_LEN +1
*/
int find_prop_node(bucket_tree_node_t *bkt_tree, bucket_tree_node_t **ret, u_8 *node_str)
{
    bucket_tree_node_t *tmp = bkt_tree;
    while(NULL != tmp)
    {
        if(is_node_leaf(tmp))
        {
            *ret = tmp;
            return 0;
        }
        if (1 == cmp_node_str(tmp->range_end_str, node_str))
        {
            tmp = tmp->l;
        }
        else //if (-1 == cmp_node_str(tmp->range_end_str, node_str))
        {
            tmp = tmp->r;
        }
    }
    return -1;
}
/**
* bits range from 0,160
*/
void generate_str_by_bit(int bits, u_8 *out_str)
{
    u_8 tmp_char[NODE_STR_LEN + 1] ={0};
    if (0 == bits)
    {
        memcpy(out_str, tmp_char, NODE_STR_LEN + 1);
        return;
    }
    int idx = bits / sizeof(u_8);
    int rest = bits % sizeof(u_8);
    tmp_char[NODE_STR_LEN - idx] |= (u_8)(1U << (rest) );
    memcpy(out_str, tmp_char, NODE_STR_LEN + 1);
}

void big_bit_add(u_8 *a, u_8 *b, u_8 *ret)
{
    u_8 add_more = 0;
    for (int i = NODE_STR_LEN; i >= 0; --i)
    {
        u_16 t = a[i] + b[i] + add_more;
        ret[i] = (u_8)t;
        add_more = (t>>8);
    }
}

void big_bit_half(u_8 *a, u_8 *ret)
{
    u_8 lowest_bit = 0;
    for (int i = 0; i <= NODE_STR_LEN; ++i)
    {
        ret[i] = (a[i] >> 1) | (lowest_bit << 7);
        lowest_bit = a[i] & 0x1;
    }
}

void generate_middle_str(u_8 *ret, u_8 *start_str, u_8 *end_str)
{
    u_8 _add[NODE_STR_LEN + 1] ={0};
    big_bit_add(start_str, end_str, _add);
    big_bit_half(_add, ret);
}
/**
* here node_str is NODE_STR_LEN +1
*/
int insert_into_bkt(node_t *node, bucket_tree_node_t *bkt_tree, u_8 *node_str, u_8 *node_ip, u_8 *node_port, u_8 *self_node_id)
{
    bool include_self = false;
    int first_empty_slot = -1;
    for (int i = 0; i < BUCKET_SIZE; ++ i)
    {
        u_8 t[NODE_STR_LEN+1] = {0};
        convert_node_str(bkt_tree->peer_nodes[i].node_str, t);

        if (0 == cmp_node_str(node_str, t )) //same node_str
        {
            if (bkt_tree->peer_nodes[i].status == NOT_USE)
            {
                printf("[ERROR] should'nt happen \n");
                bkt_tree->peer_nodes[i].status = IN_USE;
            }
            return 0 ;
        }
        if (bkt_tree->peer_nodes[i].status == NOT_USE && -1 == first_empty_slot )
        {
            first_empty_slot = i;
        }
        if (0 == cmp_node_str(self_node_id, bkt_tree->peer_nodes[i].node_str))
        {
            include_self = true;
            printf("[debug] include self true\n");
        }
    }

    if (-1 != first_empty_slot )
    {
        bkt_tree->peer_nodes[first_empty_slot].status = IN_USE;
        memcpy(bkt_tree->peer_nodes[first_empty_slot].node_str, node_str +1 , NODE_STR_LEN);
        memcpy(bkt_tree->peer_nodes[first_empty_slot].node_ip, node_ip, NODE_STR_IP_LEN);
        memcpy(bkt_tree->peer_nodes[first_empty_slot].node_port, node_port, NODE_STR_PORT_LEN);
        time_t t ;
        time(&t);
        bkt_tree->peer_nodes[first_empty_slot].update_time = t;
        node->route_num += 1;
        printf("[debug] insert one to NOT_USE slot\n");
        return 0;
    }
    if (false == include_self)
    {
        return 0;
    }
    // bkt should split into two bucket
    bkt_tree->l = (bucket_tree_node_t*)malloc(sizeof(bucket_tree_node_t));
    bkt_tree->r = (bucket_tree_node_t*)malloc(sizeof(bucket_tree_node_t));
    if (NULL == bkt_tree || NULL == bkt_tree)
    {
        free(bkt_tree->l);
        free(bkt_tree->r);
        bkt_tree->l = NULL;
        bkt_tree->r = NULL;
        printf("[debug] mem alloc err\n");
        return -1;
    }
    //old bkt start = end
    u_8 tmp_middle_str[NODE_STR_LEN +1] ={0};
    generate_middle_str(tmp_middle_str, bkt_tree->range_start_str, bkt_tree->range_end_str);

    memcpy(bkt_tree->range_start_str, tmp_middle_str, NODE_STR_LEN+1);
    memcpy(bkt_tree->range_end_str, tmp_middle_str, NODE_STR_LEN+1);

    memcpy(bkt_tree->l->range_start_str, bkt_tree->range_start_str, NODE_STR_LEN +1);
    memcpy(bkt_tree->l->range_end_str, tmp_middle_str, NODE_STR_LEN +1);

    memcpy(bkt_tree->r->range_start_str, tmp_middle_str, NODE_STR_LEN+1);
    memcpy(bkt_tree->r->range_end_str, bkt_tree->range_end_str, NODE_STR_LEN+1);

    int ll = 0;
    int rr = 0;
    for (int j = 0; j < BUCKET_SIZE; ++j)
    {
        u_8 t[NODE_STR_LEN+1] = {0};
        convert_node_str(bkt_tree->peer_nodes[j].node_str, t);
        if (1 == cmp_node_str(tmp_middle_str, t))
        {
            memcpy(&bkt_tree->l->peer_nodes[ll++], &bkt_tree->peer_nodes[j], sizeof(peer_info_t));
            printf("[debug] re-insert one to NOT_USE slot\n");
        }
        else
        {
            memcpy(&bkt_tree->r->peer_nodes[rr++], &bkt_tree->peer_nodes[j], sizeof(peer_info_t));
        }
    }
    //insert cur new node_str
    if (1 == cmp_node_str(tmp_middle_str, node_str))
    {
        memcpy(bkt_tree->l->peer_nodes[ll].node_str, node_str +1, NODE_STR_LEN);
        memcpy(bkt_tree->l->peer_nodes[ll].node_ip, node_ip, NODE_STR_IP_LEN);
        memcpy(bkt_tree->l->peer_nodes[ll].node_port, node_port, NODE_STR_PORT_LEN);
        bkt_tree->l->peer_nodes[ll].status = IN_USE;
        time_t t ;
        time(&t);
        bkt_tree->r->peer_nodes[ll].update_time = t;
    }
    else
    {
        memcpy(bkt_tree->r->peer_nodes[rr].node_str, node_str +1, NODE_STR_LEN);
        memcpy(bkt_tree->r->peer_nodes[rr].node_ip, node_ip, NODE_STR_IP_LEN);
        memcpy(bkt_tree->r->peer_nodes[rr].node_port, node_port, NODE_STR_PORT_LEN);
        bkt_tree->r->peer_nodes[rr].status = IN_USE;
        time_t t ;
        time(&t);
        bkt_tree->r->peer_nodes[rr].update_time = t;
    }
    node->route_num += 1;
    return 0;

}

/**
* here node_str is NODE_STR_LEN
*/
int insert_into_bucket_tree(node_t *node, bucket_tree_node_t **bkt_tree,u_8 *node_str, u_8 *node_ip, u_8 *node_port, u_8 *self_node_id)
{
    //bucket_tree_node_t *bkt_tree = *root;
    if (NULL == *bkt_tree)
    {
        *bkt_tree = (bucket_tree_node_t*)malloc(sizeof(bucket_tree_node_t));
        if (NULL == *bkt_tree)
        {
            printf("malloc err\n");
            return -1;
        }
        memset((*bkt_tree), 0, sizeof(bucket_tree_node_t));
        memcpy((*bkt_tree)->peer_nodes[0].node_str, node_str, NODE_STR_LEN * sizeof(char));
        memcpy((*bkt_tree)->peer_nodes[0].node_ip, node_ip, sizeof(u_32));
        memcpy((*bkt_tree)->peer_nodes[0].node_port, node_port, sizeof(u_16));
        (*bkt_tree)->peer_nodes[0].status = IN_USE;
        time_t t ;
        time(&t);
        (*bkt_tree)->peer_nodes[0].update_time = t;

        u_8 start_str[NODE_STR_LEN +1 ] = {0};
        generate_str_by_bit(0, start_str);
        memcpy((*bkt_tree)->range_start_str, start_str, NODE_STR_LEN +1 );

        u_8 end_str[NODE_STR_LEN +1 ] = {0};
        generate_str_by_bit(160, end_str);
        memcpy((*bkt_tree)->range_end_str, end_str, NODE_STR_LEN +1 );

        (*bkt_tree)->l = NULL;
        (*bkt_tree)->r = NULL;
        node->route_num += 1;

        printf("[debug] insert one to mem.\n");

        return 0;

    }

    bucket_tree_node_t *the_node = NULL;
    u_8 dst[NODE_STR_LEN+1] = {0};
    convert_node_str(node_str, dst);
    int ret = find_prop_node(*bkt_tree, &the_node, dst);
    if (-1 == ret)
    {
        printf("Not find !! must be err\n");
        return -1;
    }

    ret = insert_into_bkt(node, the_node, dst, node_ip, node_port, self_node_id);
    printf("----insert ip : %hhu.%hhu.%hhu.%hhu, port: %hu\n", node_ip[0],node_ip[1],node_ip[2],node_ip[3], *node_port);
    if (-1 == ret)
    {
        printf("Insert into bkt failed\n");
        return -1;
    }

    return 0;
}

int save_route_tbl_to_file(node_t* node)
{
    FILE *fp = NULL;
    fp = fopen("dht_route_s.dat", "wb+");
    if (NULL == fp)
    {
        printf("open write file failed\n");
        return -1;
    }

    tree_stack_t t_stack;
    tree_stack_t *t=&t_stack;
    init_tree_stack(t);

    bucket_tree_node_t *root = node->bkt_tree;
    printf("root is null ? %d\n", root ==NULL);

    fwrite(node->node_id, 1, NODE_STR_LEN, fp);
    fwrite(&node->route_num, 1, sizeof(int),fp);

    tree_push(t, &root);

    while(!tree_stack_is_empty(t))
    {
        bucket_tree_node_t *tmp = NULL;
        tree_pop(t, &tmp);
        if (is_node_leaf(tmp))
        {
            for (int i = 0; i < BUCKET_SIZE; ++i)
            {
                if (tmp->peer_nodes[i].status == IN_USE)
                {
                    fwrite(tmp->peer_nodes[i].node_str, 1, NODE_STR_LEN , fp);
                    fwrite(tmp->peer_nodes[i].node_ip, 1, NODE_STR_IP_LEN , fp);
                    fwrite(tmp->peer_nodes[i].node_port, 1, NODE_STR_PORT_LEN , fp);
                    //char _ip_out [20] = {0};
                    //inet_bin_to_string(tmp->peer_nodes[i].node_ip, _ip_out);
                    u_16 _port = *(u_16*)(&tmp->peer_nodes[i].node_port);
                    u_8 *_ip = tmp->peer_nodes[i].node_ip;
                    printf("write ip: %hhu.%hhu.%hhu.%hhu, port :%hu\n ",_ip[0], _ip[1], _ip[2], _ip[3] , _port);
                }
            }
        }
        else
        {
            tree_push(t, &tmp->l);
            tree_push(t, &tmp->r);
        }
    }

    fclose(fp);
    fp = NULL;

    return 0;
}

int read_route_tbl_frm_config(node_t *node)
{
    FILE *fp = NULL;
    fp = fopen("dht_route_s.dat", "rb");
    if (NULL == fp)
    {
        printf("open read file failed\n");
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    if (0 == file_size)
    {
        printf("file is empty!\n");
        fclose(fp);
        return 0;
    }
    //read self node id
    //unsigned char self_node_id[NODE_STR_LEN] = {0};
    fseek(fp, 0, SEEK_SET);
    size_t read_len = fread(node->node_id, 1, NODE_STR_LEN, fp);
    if (read_len != NODE_STR_LEN)
    {
        printf("read self node id err(%d)\n", read_len);
        fclose(fp);
        return -1;
    }

    //read route num
    int route_num = 0;
    read_len = fread(&route_num,  1, sizeof(int), fp);
    if (read_len != sizeof(int))
    {
        printf("read route num err(%u)\n", read_len);
        fclose(fp);
        return -1;
    }

    printf("read route num : %d\n", route_num);
    //node->route_num = route_num;

    //read route tbl
    u_8 * tmp_node_str = NULL;
    int str_len = (NODE_STR_LEN + NODE_STR_IP_LEN + NODE_STR_PORT_LEN);
    size_t buf_size = str_len * sizeof(unsigned char) * route_num;
    tmp_node_str = (u_8*)malloc(buf_size);
    if (NULL == tmp_node_str)
    {
        fclose(fp);
        printf("malloc mem (size: %u) err\n", buf_size);
        return -1;
    }

    read_len = 0;
    read_len = fread(tmp_node_str, str_len * sizeof(unsigned char), route_num, fp);
    if (read_len != route_num)
    {
        printf("read err\n,real node cnt : %d\n", read_len);
        free(tmp_node_str);
        fclose(fp);
        return -1;
    }

    //update to route tbl in mem;
    //insert to bucket tree
    u_8 * temp =(u_8*)tmp_node_str;
    int ret = -1;
    for (int i= 0; i < route_num; ++i)
    {
        u_8 *node_str = temp;
        u_8 *node_ip = temp + NODE_STR_LEN;
        u_8 *node_port = temp + NODE_STR_LEN + NODE_STR_IP_LEN;
        ret = insert_into_bucket_tree(node, &node->bkt_tree, node_str, node_ip, node_port, node->node_id);
        if (-1 == ret)
        {
            free(tmp_node_str);
            fclose(fp);
            return -1;
        }

        temp += (NODE_STR_LEN+ NODE_STR_IP_LEN + NODE_STR_PORT_LEN);
    }
    free(tmp_node_str);
    fclose(fp);

    if (route_num != node->route_num)
    {
        printf("[ERROR] not equal! read from file (%d), inserted route_num (%d)\n", route_num, node->route_num);
    }

    return 0;
}

int init_route_table(node_t*node)
{
    int ret = -1;
    ret = read_route_tbl_frm_config(node);
    /**
    * if config has route , do not use bootstrap node to init, etc. dont below ping op.
    */
    if (NULL != node->bkt_tree)
    {
        printf("read route success.\n");
        return 0;
    }
    printf("start init route table by find node!\n");
    //ping bootstrap node
    sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(6881);
    //remote_addr.sin_port = htons(49246);
    //remote_addr.sin_addr.S_un.S_addr = (inet_addr("67.215.246.10"));
    remote_addr.sin_addr.S_un.S_addr = (inet_addr("87.98.162.88"));
    ret = find_node(node->send_socket,&remote_addr,node->node_id, node->node_id);
    //ret = ping_node(node->send_socket,&remote_addr,node->node_id);
     int try_times = 100;
     while (try_times -- > 0)
     {
         printf("rcv msg times %d:\n", try_times);
         ret = rcv_msg(node->send_socket, node);
         if (ret == OK)
         {
             break;
         }
         /*ret = rcv_msg(node->recv_socket, node);
         if (ret == OK)
         {
             break;
         }*/
         Sleep(20);
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

char type_desc[Y_TYPE_BUTT][30]=
{
    "BEGIN",
    "Y_TYPE_Q",
    "Y_TYPE_R",//response
    "Y_TYPE_E", // response err
    "Y_TYPE_PING",
    "Y_TYPE_PING_RSP",
    "Y_TYPE_FIND_NODE",
    "Y_TYPE_FIND_NODE_RSP",
    "Y_TYPE_GET_PEER",
    "Y_TYPE_GET_PEER_RSP",
    "Y_TYPE_ANNOUNCE_PEER",
    "Y_TYPE_ANNOUNCE_PEER_RSP"
};
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
    // update route table by ping node in route table

    // receive msg from other node.

    clear_node(&node);

    printf("\ntest byte order of mingwin:\n");

    u_16 x = 0x1;
    u_8 *y = (u_8*)&x;

    int type = 0;
    printf("lower address of x is: %hhu, higher address of x is %hhu:\n", *y, *(y+1));


    char test_ping[]="d1:ad2:id20:abcdefghij0123456789e1:q4:ping1:t2:aa1:y1:qe";
    u_32 pos = 0;
    type = 0;
    memset(&dict, 0, sizeof(ben_dict_t));
    ben_coding(test_ping,strlen(test_ping),&pos);
    get_rsp_msg_type(&dict, &type);
    printf("get test_ping msg type  : %d -> %s\n", type, type_desc[type]);

    char test_ping_rsp[]="d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:pa1:y1:re";
    pos = 0;
    type = 0;
    memset(&dict, 0, sizeof(ben_dict_t));
    ben_coding(test_ping_rsp,strlen(test_ping),&pos);
    get_rsp_msg_type(&dict, &type);
    printf("get test_ping_rsp msg type : %d -> %s\n", type, type_desc[type]);


    char test_find_node_req[]="d1:ad2:id20:abcdefghij01234567896:target20:mnopqrstuvwxyz123456e1:q9:find_node1:t2:fa1:y1:qe";
    pos = 0;
    type = 0;
    memset(&dict, 0, sizeof(ben_dict_t));
    ben_coding(test_find_node_req,strlen(test_find_node_req),&pos);
    get_rsp_msg_type(&dict, &type);
    printf("get test_find_node_req msg type : %d -> %s\n", type, type_desc[type]);

    char test_find_node_rsp[]="d1:rd2:id20:0123456789abcdefghij5:nodes9:def456...e1:t2:fa1:y1:re";
    pos = 0;
    type = 0;
    memset(&dict, 0, sizeof(ben_dict_t));
    ben_coding(test_find_node_rsp,strlen(test_find_node_rsp),&pos);
    get_rsp_msg_type(&dict, &type);
    printf("get test_find_node_rsp msg type : %d -> %s\n", type, type_desc[type]);


    char test_get_peers_req[]="d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz123456e1:q9:get_peers1:t2:ga1:y1:qe";
    pos = 0;
    type = 0;
    memset(&dict, 0, sizeof(ben_dict_t));
    ben_coding(test_get_peers_req,strlen(test_get_peers_req),&pos);
    get_rsp_msg_type(&dict, &type);
    printf("get test_get_peers_req msg type : %d -> %s\n", type, type_desc[type]);

    char test_get_peers_rsp1[]="d1:rd2:id20:abcdefghij01234567895:token8:aoeusnth6:valuesl6:axje.u6:idhtnmee1:t2:ga1:y1:re";
    pos = 0;
    type = 0;
    memset(&dict, 0, sizeof(ben_dict_t));
    ben_coding(test_get_peers_rsp1,strlen(test_get_peers_rsp1),&pos);
    get_rsp_msg_type(&dict, &type);
    printf("get test_get_peers_rsp1 msg type : %d -> %s\n", type, type_desc[type]);

    char test_get_peers_rsp2[]="d1:rd2:id20:abcdefghij01234567895:nodes9:def456...5:token8:aoeusnthe1:t2:ga1:y1:re";
    pos = 0;
    type = 0;
    memset(&dict, 0, sizeof(ben_dict_t));
    ben_coding(test_get_peers_rsp2,strlen(test_get_peers_rsp2),&pos);
    get_rsp_msg_type(&dict, &type);
    printf("get test_get_peers_rsp2 msg type : %d -> %s\n", type, type_desc[type]);


    char test_announce_req[]="d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz1234564:porti6881e5:token8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe";
    pos = 0;
    type = 0;
    memset(&dict, 0, sizeof(ben_dict_t));
    ben_coding(test_announce_req,strlen(test_announce_req),&pos);
    get_rsp_msg_type(&dict, &type);
    printf("get test_announce_req msg type : %d -> %s\n", type, type_desc[type]);

    char test_announce_rsp[]="d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re";
    pos = 0;
    type = 0;
    memset(&dict, 0, sizeof(ben_dict_t));
    ben_coding(test_announce_rsp,strlen(test_announce_rsp),&pos);
    get_rsp_msg_type(&dict, &type);
    printf("get test_announce_rsp msg type : %d -> %s\n", type, type_desc[type]);

    return 0;
}
