//
// Created by xx on 2018/8/26.
//

#include <stdio.h>
#include "dht_spider.h"
#include "bt_parser.h"
#include <stdarg.h>
#include "hash_tbl.h"
#include <time.h>

u_32 g_req_id;
u_32 g_find_node_id;
hash_tbl tmp_map, *timer_map = &tmp_map;
//hash_tbl tmp_map2, *msg_ctx_map = &tmp_map2;
hash_tbl htmp, *torrent_hash_map = &htmp;
unsigned int g_timer_id;

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
int find_prop_node(bucket_tree_node_t *bkt_tree, bucket_tree_node_t **ret, u_8 *node_str);
void print_info_hash(char info_hash[], int info_hash_len);

char print_buff[1000]={0};

//hash_map timer_map;
//hash_map *t_map = &timer_map;

void dht_print_helper(const char* fmt, ...)
{
    struct tm *p;
    time_t timep;
    time(&timep);
    p = localtime(&timep);
    va_list ap;
    va_start(ap,fmt);
    memset(print_buff, 0, 1000);
    vsprintf(print_buff, fmt, ap);
    printf("[%4d\-%02d-%02d %02d:%02d:%02d] %s",p->tm_year+1900, p->tm_mon+1, p->tm_mday, p->tm_hour,p->tm_min,p->tm_sec,print_buff);
    va_end(ap);
}



int __add_to_msg_queue(msg_t* m, msg_q_mgr_t *q_mgr)
{

    if (q_mgr->snd_q.msg_cnt >= MAX_MSG_QUEUE_SIZE)
    {
        //free(m);
        //m = NULL;
        dht_print("[warning] snd_q is full\n");
        return ERR;
    }
    return OK;
}
void __put_msg_ctx_map(hash_tbl *m, u_32 tid, refresh_route_ctx_t* msg_ctx)
{
    //dht_print("++++++++++add a tid (%u) ++++\n", tid);
    if (m->used >= MAX_MSG_QUEUE_SIZE)
    {
        //dht_print("ctx full.........\n");
        return;
    }
    map_entry e = {0};
    e.key = (void*)malloc(sizeof(u_32));
    u_32 *tmp_tid = (u_32*)e.key;
    *tmp_tid = tid;
    e.val = (void*)msg_ctx;
    map_put(m, &e);
}

void print_node_str(u_8 *str, int len)
{
    for(int i = 0; i < len; ++i)
    {
        printf("%hu|", str[i]);
    }
    printf("\n");
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

u_8 tid = 0;

msg_q_mgr_t q_mgr;

u_16 dht_ntohs(u_8* x)
{
    u_16 r = 0;
    u_8 *tmp = (u_8*)&r;
    tmp[0] = x[1];
    tmp[1] = x[0];
    return r;
}

u_32 dht_ntohl(u_8 *ip)
{
    u_32 r = 0;
    u_8 *tmp = (u_8*)&r;
    tmp[0] = ip[3];
    tmp[1] = ip[2];
    tmp[2] = ip[1];
    tmp[3] = ip[0];
    return r;
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

int tree_stack_is_empty(tree_stack_t *s)
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

int ping_node(node_t *node, struct sockaddr_in* addr, unsigned char* self_node_id, peer_info_t *peer_addr)
{
    //d1:ad2:id20:abcdefghij0123456789e1:q4:ping1:t2:aa1:y1:qe
    char ping_str_start[100] = "d1:ad2:id20:";
    char *tmp_str = ping_str_start;
    tmp_str += strlen(ping_str_start);

    memcpy(tmp_str, self_node_id, NODE_STR_LEN);
    tmp_str += NODE_STR_LEN;

    char ping_str_end[100] = "e1:q4:ping1:t";
    //"2:p";
    size_t s_len = strlen(ping_str_end);
    memcpy(tmp_str, ping_str_end, s_len);
    tmp_str += s_len;

    char tid_len_str[8] = {0};

    u_32 ping_tid = g_req_id ++;
    char ping_tid_str[8] = {0};
    sprintf(ping_tid_str, "%u", ping_tid);
    s_len = strlen(ping_tid_str);

    sprintf(tid_len_str, "%hu", (s_len+1));
    size_t ss_len = strlen(tid_len_str);

    memcpy(tmp_str, tid_len_str, ss_len);
    tmp_str += ss_len;

    memcpy(tmp_str, ":p", 2);
    tmp_str += 2;

    memcpy(tmp_str, ping_tid_str, s_len);
    tmp_str += s_len;

    char ping_str_end2[]="1:y1:qe";
    s_len = strlen(ping_str_end2);
    memcpy(tmp_str, ping_str_end2, s_len);
    tmp_str += s_len;

    msg_t *m = (msg_t*)malloc(sizeof(msg_t));
    if (NULL == m)
    {
        dht_print("[ERROR] ################ alloc mem err\n");
        return ERR;
    }
    memset(m, 0, sizeof(msg_t));
    m->msg_type = SEND_MSG;
    m->addr = *addr;
    m->buf_len = (int)(tmp_str - ping_str_start);
    m->tid = ping_tid;
    memcpy(m->buf, ping_str_start, m->buf_len);
    //dht_print("[debug] ping msg {%s} len is (%d)\n",m->buf , m->buf_len);

    int ret = __add_to_msg_queue(m, &q_mgr);
    if (ret != OK)
    {
        free(m);
        m = NULL;
        return ERR;
    }

    refresh_route_ctx_t * ctx  = (refresh_route_ctx_t*)malloc(sizeof(refresh_route_ctx_t));
    if (NULL == ctx)
    {
        free(m);
        m = NULL;
        dht_print("[FATAL] can not alloc mem\n");
        return ERR;
    }
    ctx->op_type = Y_TYPE_PING;
    ctx->timeout_sec = MSG_TIMEOUT_INTERVAL;
    ctx->route_info = peer_addr;

    sendto(node->recv_socket, m->buf, m->buf_len,0 , (struct sockaddr*)addr, sizeof(struct sockaddr_in));

    __put_msg_ctx_map(node->msg_ctx_map,ping_tid, ctx);

    return OK;
}

int query_node()
{
    return OK;
}
/**
* find node :the t value,start with f.
*/
int find_node(node_t *node,struct sockaddr_in* addr, u_8 * id, u_8 *tgt,  peer_info_t *peer_addr)
{
    //d1:ad2:id20:abcdefghij01234567896:target20:mnopqrstuvwxyz123456e1:q9:find_node1:t2:aa1:y1:qe
    char str1[] = "d1:ad2:id20:";
    char str2[] = "6:target20:";
    char str3[] = "e1:q9:find_node1:t";
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

    //2:f";

    u_32 find_node_tid = g_req_id ++;
    char find_node_tid_str[8] = {0};
    sprintf(find_node_tid_str, "%u", find_node_tid);
    size_t find_node_tid_len = strlen(find_node_tid_str);

    char tid_len_len[8] = {0};
    sprintf(tid_len_len, "%hu", (find_node_tid_len + 1) );
    size_t s_len = strlen(tid_len_len);

    memcpy(send_str, tid_len_len, s_len);
    send_str += s_len;

    memcpy(send_str, ":f", 2);
    send_str += 2;

    memcpy(send_str, find_node_tid_str, find_node_tid_len);
    send_str += find_node_tid_len;

    //*send_str ='d';// tid ++;
    //send_str +=1;

    memcpy(send_str, str4, strlen(str4));
    send_str += strlen(str4);

    int send_len = (send_str - snd_str);

    sendto(node->recv_socket,(char *)snd_str, send_len, 0,(struct sockaddr*)addr, sizeof(struct sockaddr));
    //dht_print("send find node msg(len:%d): %s\n", send_len, snd_str);
    msg_t *m = (msg_t*)malloc(sizeof(msg_t));
    m->msg_type = SEND_MSG;
    m->addr = *addr;
    m->buf_len = send_len;//strlen("d1:ad2:id20:abcdefghij0123456789e1:q4:ping1:t2:aa1:y1:qe");
    memcpy(m->buf, snd_str, m->buf_len);


    if (peer_addr) // root节点 这里传空，不保存ctx
    {
        refresh_route_ctx_t * ctx  = (refresh_route_ctx_t*)malloc(sizeof(refresh_route_ctx_t));
        if (NULL == ctx)
        {
            free(m);
            m = NULL;
            dht_print("[FATAL] can not alloc mem\n");
            return ERR;
        }

        ctx->op_type = Y_TYPE_FIND_NODE;
        ctx->timeout_sec = MSG_TIMEOUT_INTERVAL;
        ctx->route_info = peer_addr;
        __put_msg_ctx_map(node->msg_ctx_map,find_node_tid, ctx);
    }

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
            //dht_print("\nget rsp t: %s\n", e->p.dict_val_ref->str);
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
            //dht_print("\nget rsp q: %s\n", e->p.dict_val_ref->str);
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
                dht_print("[warning] get rsp q: %s\n", e->p.dict_val_ref->str);
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
    * ping请求={"t":"aa", "y":"q","q":"ping", "a":{"id":"abcdefghij0123456789"}}

B编码=d1:ad2:id20:abcdefghij0123456789e1:q4:ping1:t2:aa1:y1:qe

回复={"t":"aa", "y":"r", "r":{"id":"mnopqrstuvwxyz123456"}}

B编码=d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re


find_node请求={"t":"aa", "y":"q","q":"find_node", "a":{"id":"abcdefghij0123456789","target":"mnopqrstuvwxyz123456"}}

B编码=d1:ad2:id20:abcdefghij01234567896:target20:mnopqrstuvwxyz123456e1:q9:find_node1:t2:aa1:y1:qe

回复={"t":"aa", "y":"r", "r":{"id":"0123456789abcdefghij", "nodes":"def456..."}}

B编码=d1:rd2:id20:0123456789abcdefghij5:nodes9:def456...e1:t2:aa1:y1:re


get_peers请求={"t":"aa", "y":"q","q":"get_peers", "a":{"id":"abcdefghij0123456789","info_hash":"mnopqrstuvwxyz123456"}}

B编码=d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz123456e1:q9:get_peers1:t2:aa1:y1:qe

回复peers ={"t":"aa", "y":"r", "r":{"id":"abcdefghij0123456789", "token":"aoeusnth","values": ["axje.u", "idhtnm"]}}

B编码=d1:rd2:id20:abcdefghij01234567895:token8:aoeusnth6:valuesl6:axje.u6:idhtnmee1:t2:aa1:y1:re

回复最接近的nodes= {"t":"aa", "y":"r", "r":{"id":"abcdefghij0123456789", "token":"aoeusnth","nodes": "def456..."}}

B编码=d1:rd2:id20:abcdefghij01234567895:nodes9:def456...5:token8:aoeusnthe1:t2:aa1:y1:re


announce_peers请求={"t":"aa", "y":"q","q":"announce_peer", "a":{"id":"abcdefghij0123456789","info_hash":"mnopqrstuvwxyz123456", "port":6881, "token": "aoeusnth"}}

B编码=d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz1234564:porti6881e5:token8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe

回复={"t":"aa", "y":"r", "r":{"id":"mnopqrstuvwxyz123456"}}

B编码=d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re


    */
    u_16 t = 0;
    str_ele_t *e = dict->e[0].p.dict_val_ref;
    while (NULL != e)
    {
        if (0 == strcmp(e->str, "y"))
        {
            //dht_print("\nget rsp y: %s\n", e->p.dict_val_ref->str);
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
    dht_print("not find y value!\n");
    print_dict(dict);
    return -1;
}

int get_ip_in_ping_rsp(ben_dict_t *dict, u_32 *ip, u_16 *port)
{
    str_ele_t *e = dict->e[0].p.dict_val_ref;
    while (NULL != e)
    {
        if (0 == strcmp(e->str, "ip"))
        {
            dht_print("get ping rsp ip: %s\n", e->p.dict_val_ref->str);
            memcpy(ip, e->p.dict_val_ref->str, sizeof(u_32));
            memcpy(port,e->p.dict_val_ref->str + sizeof(u_32), sizeof(u_16));
            return 0;
        }
        e = e->p.list_next_ref;
    }
    dht_print("not find ip value!\n");
    return -1;
}

int get_id_in_ping_rsp(ben_dict_t *dict, u_8 *id)
{
    str_ele_t *e = dict->e[0].p.dict_val_ref;
    while (NULL != e)
    {
        if (0 == strcmp(e->str, "r"))
        {
            dht_print("get ping rsp r: %s\n", e->p.dict_val_ref->str);
            str_ele_t *e1 = e->p.dict_val_ref->p.dict_val_ref;
            while (NULL != e1)
            {
                if (0 == strcmp(e1->str, "id"))
                {
                    dht_print("get ping rsp id of r: %s\n", e1->p.dict_val_ref->str);
                    memcpy(id, e1->p.dict_val_ref->str, 20*sizeof(u_8));
                    return 0;
                }
            }

        }
        e = e->p.list_next_ref;
    }
    dht_print("not find id value!\n");
    return -1;
}

void inet_bin_to_string(u_32 ip, char output[])
{
    unsigned char *tmp = (unsigned char*)&ip;
    char buf[4] = {0};
    for (int i = 0; i < 4; ++i)
    {
        memset(buf, 0, 4);
        sprintf(buf, "%hu", tmp[i]);
        strcat(output, buf);
        if (i != 3)
            strcat(output, ".");
    }
}

void handle_on_ping(struct sockaddr_in *rcv_addr, ben_dict_t *dict, node_t *node)
{//B编码=d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re
    dht_print("get a ping req...\n");
    str_ele_t *e = dict->e[0].p.dict_val_ref;
    char peer_tid[4] = {0};
    int peer_tid_len = 0;
    while (NULL != e)
    {
        if (0 == strcmp(e->str, "t"))
        {
            peer_tid_len = e->p.dict_val_ref->str_len;
            if (peer_tid_len > 4)
            {
                dht_print("erro tid len(%d)\n", peer_tid_len);
                break;
            }
            dht_print("\nget a ping req tid: %s, len (%d)\n", e->p.dict_val_ref->str, peer_tid_len);
            memcpy(peer_tid, e->p.dict_val_ref->str,  peer_tid_len);
            break;
        }
        e = e->p.list_next_ref;
    }
    if (0 == peer_tid_len)
    {
        dht_print("[E] not find tid.\n");
        return;
    }

    //d1:ad2:id20:abcdefghij0123456789e1:q4:ping1:t2:aa1:y1:qe
    //d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re
    char ping_rsp_str_start[100] = "d1:ad2:id20:";
    int start_str_len = strlen((char*)ping_rsp_str_start);
    memcpy(ping_rsp_str_start+ start_str_len, node->node_id, NODE_STR_LEN);

    memcpy(ping_rsp_str_start+start_str_len+NODE_STR_LEN, "e1:t2:", 6);
    memcpy(ping_rsp_str_start+start_str_len+NODE_STR_LEN +6, peer_tid, peer_tid_len);
    memcpy(ping_rsp_str_start+start_str_len+NODE_STR_LEN +6 + peer_tid_len, "1:y1:re", 7);

    msg_t *m = (msg_t*)malloc(sizeof(msg_t));
    m->addr = *rcv_addr;
    m->buf_len = strlen("d1:ad2:id20:abcdefghij0123456789e1:q4:ping1:t2:aa1:y1:qe");
    memcpy(m->buf, ping_rsp_str_start, m->buf_len);

    if (q_mgr.snd_q.msg_cnt >= MAX_MSG_QUEUE_SIZE)
    {
        free(m);
        m = NULL;
        dht_print("[warning] snd_q is full\n");
        return;
    }

    u_32 frm_ip = (rcv_addr->sin_addr.S_un.S_addr);
    char out[20] ={0};
    inet_bin_to_string(frm_ip, out);

    dht_print("sending (%s:%u) a on ping msg:(%s)\n",out,ntohs(rcv_addr->sin_port), ping_rsp_str_start);
    sendto(node->recv_socket,ping_rsp_str_start,m->buf_len, 0, (struct sockaddr*)rcv_addr, sizeof(struct sockaddr_in) );
}

void handle_on_find_node(struct sockaddr_in *rcv_addr, ben_dict_t *dict, node_t *node)
{
    //dht_print("get a find node req...\n");
    str_ele_t *e = dict->e[0].p.dict_val_ref;
    char peer_tid[4] = {0};
    char peer_nid[NODE_STR_LEN] = {0};
    char peer_tgtid[NODE_STR_LEN] = {0};
    int peer_tid_len = 0;

    int find_peer_id = 0, find_peer_tgt = 0;
    while (NULL != e)
    {
        if ('t' == e->str[0])
        {
            peer_tid_len = e->p.dict_val_ref->str_len;
            if (peer_tid_len > 4)
            {
                dht_print("erro tid len(%d)\n", peer_tid_len);
                break;
            }
            dht_print("get a find node req tid: %s, len (%d)\n", e->p.dict_val_ref->str, peer_tid_len);
            memcpy(peer_tid, e->p.dict_val_ref->str,  peer_tid_len);
            //break;
        }
        if (e->str[0] == 'a')
        {
            str_ele_t *e1 = e->p.dict_val_ref->p.dict_val_ref;
            while(NULL != e1)
            {
                if (0 == strcmp(e1->str, "id"))
                {
                    memcpy(peer_nid, e1->str, e1->str_len);
                    find_peer_id = 1;
                }
                if (0 == strcmp(e1->str, "target"))
                {
                    memcpy(peer_tgtid, e1->str, e1->str_len);
                    find_peer_tgt = 1;
                }
                e1 = e1->p.list_next_ref;
            }

        }
        e = e->p.list_next_ref;
    }

    if (!(find_peer_id && find_peer_tgt && peer_tid_len != 0))
    {
        dht_print("[ERROR] not find key word! find peer id (%d), find peer tgt (%d), peer tid len (%d)\n", find_peer_id, find_peer_tgt, peer_tid_len);
        return;
    }
    //回复={"t":"aa", "y":"r", "r":{"id":"0123456789abcdefghij", "nodes":"def456..."}}
    //B编码=d1:rd2:id20:0123456789abcdefghij5:nodes9:def456...e1:t2:aa1:y1:re
    char rsp_str[500]="d1:rd2:id20:";
    char *tmp_str = rsp_str;
    tmp_str += strlen(rsp_str);
    memcpy(tmp_str, node->node_id, NODE_STR_LEN);
    tmp_str += NODE_STR_LEN;

    memcpy(tmp_str, "5:nodes", 7 );
    tmp_str += 7;

    int nearest_node_num = 0;

///////////////////////////////////////////
    bucket_tree_node_t *nearest_node = NULL;
    int ret = find_prop_node(node->bkt_tree, &nearest_node, (u_8 *)peer_tgtid);
    if (-1 == ret)
    {
        dht_print("[ERROR] not find tgt node bkt!\n");
        return;
    }
    char nodes_buffer[500] = {0};
    char *tmp_nodes_buf = nodes_buffer;
    for (int j = 0; j < BUCKET_SIZE; ++j)
    {
        if (nearest_node->peer_nodes[j].status != IN_USE)
        {
            continue;
        }
        memcpy(tmp_nodes_buf, nearest_node->peer_nodes[j].node_str, NODE_STR_LEN);
        tmp_nodes_buf += NODE_STR_LEN;

        memcpy(tmp_nodes_buf, nearest_node->peer_nodes[j].node_ip, NODE_STR_IP_LEN);
        tmp_nodes_buf += NODE_STR_PORT_LEN;

        memcpy(tmp_nodes_buf, nearest_node->peer_nodes[j].node_port, NODE_STR_PORT_LEN);

        nearest_node_num ++;
    }
    int nod_buf_len = nearest_node_num * (NODE_STR_LEN+NODE_STR_IP_LEN+ NODE_STR_PORT_LEN);

    char node_num_str[4] = {0};
    sprintf(node_num_str, "%d:", nod_buf_len);
    memcpy(tmp_str, node_num_str, strlen(node_num_str) );
    tmp_str += strlen(node_num_str);
    dht_print("get nearest (%s) peers!\n", node_num_str);


    memcpy(tmp_str, nodes_buffer, nod_buf_len);
    tmp_str += nod_buf_len;

    memcpy(tmp_str, "e1:t2:", 6);
    tmp_str += 6;

    memcpy(tmp_str, peer_tid, peer_tid_len);
    tmp_str += peer_tid_len;

    memcpy(tmp_str, "1:y1:re", 7);
    tmp_str += 7;
// send q------
    msg_t *m = (msg_t*)malloc(sizeof(msg_t));
    m->addr = *rcv_addr;
    m->buf_len = (tmp_str - rsp_str);
    if (m->buf_len > 500)
    {
        dht_print("[WARNING] msg too long (%d)\n", m->buf_len);
        free(m);
        m = NULL;
        return;
    }
    memcpy(m->buf, rsp_str, m->buf_len);

    if (q_mgr.snd_q.msg_cnt >= MAX_MSG_QUEUE_SIZE)
    {
        free(m);
        m = NULL;
        dht_print("[warning] snd_q is full\n");
        return;
    }

    u_32 frm_ip = (rcv_addr->sin_addr.S_un.S_addr);
    char out[20] ={0};
    inet_bin_to_string(frm_ip, out);

    dht_print("sending (%s) a on find node msg:(%s) len (%d)\n",out, rsp_str, m->buf_len);

}

void print_info_hash(char info_hash[], int info_hash_len)
{
    char hash_hex[50] ={0};

    for (int i = 0 ; i < info_hash_len; ++i)
    {
        sprintf((char*)(&hash_hex[0] +i*2),  "%02x", (unsigned char)info_hash[i]);
    }
    dht_print("\n\n[info-hash] %s ,len (%u);info_hash_len (%d)\n\n ", hash_hex, strlen(hash_hex), info_hash_len);
}

void handle_on_get_peer(struct sockaddr_in *rcv_addr, ben_dict_t *dict, node_t *node)
{
    //printf("get a  get_peer req...\n");
    //print_result(&g_stack, &g_ctx_stack);
    //get_peers请求={"t":"aa", "y":"q","q":"get_peers", "a":{"id":"abcdefghij0123456789","info_hash":"mnopqrstuvwxyz123456"}}
    //B编码=d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz123456e1:q9:get_peers1:t2:aa1:y1:qe
    str_ele_t *e = dict->e[0].p.dict_val_ref;
    char peer_tid[4] = {0};
    char peer_nid[NODE_STR_LEN] = {0};
    char info_hash[NODE_STR_LEN+1] = {0};
    int peer_tid_len = 0;
    int info_hash_len = 0;

    int find_peer_id = 0, find_peer_tgt = 0;
    while (NULL != e)
    {
        if ('t' == e->str[0])
        {
            peer_tid_len = e->p.dict_val_ref->str_len;
            if (peer_tid_len > 4)
            {
                dht_print("erro tid len(%d)\n", peer_tid_len);
                break;
            }
            dht_print("get a find node req tid: %s, len (%d)\n", e->p.dict_val_ref->str, peer_tid_len);
            memcpy(peer_tid, e->p.dict_val_ref->str,  peer_tid_len);
            //break;
        }
        if (e->str[0] == 'a')
        {
            str_ele_t *e1 = e->p.dict_val_ref->p.dict_val_ref;
            while(NULL != e1)
            {
                if (0 == strcmp(e1->str, "id"))
                {
                    memcpy(peer_nid, e1->p.dict_val_ref->str, e1->str_len);
                    find_peer_id = 1;
                }
                if (0 == strcmp(e1->str, "info_hash"))
                {
                    memcpy(info_hash, e1->p.dict_val_ref->str, e1->p.dict_val_ref->str_len);
                    find_peer_tgt = 1;
                    info_hash_len = e1->p.dict_val_ref->str_len;
                }
                e1 = e1->p.list_next_ref;
            }

        }
        e = e->p.list_next_ref;
    }

    if (!(find_peer_id && find_peer_tgt && peer_tid_len != 0))
    {
        dht_print("[ERROR] not find key word! find peer id (%d), find peer tgt (%d), peer tid len (%d)\n", find_peer_id, find_peer_tgt, peer_tid_len);
        return;
    }
    print_info_hash(info_hash, info_hash_len);
    if (info_hash_len != 20)
    {
        return;
    }
    //save to local torrent info-hash map
    map_entry en = {0};
    en.key = (void*)malloc(info_hash_len);

    map_put(node->torrent_hash_map, &en);

    //回复最接近的nodes= {"t":"aa", "y":"r", "r":{"id":"abcdefghij0123456789", "token":"aoeusnth","nodes": "def456..."}}
    //B编码=d1:rd2:id20:abcdefghij01234567895:nodes9:def456...5:token8:aoeusnthe1:t2:aa1:y1:re
    char rsp_str[500]="d1:rd2:id20:";
    char *tmp_str = rsp_str;
    memcpy(tmp_str, node->node_id, NODE_STR_LEN);
    tmp_str += NODE_STR_LEN;

    memcpy(tmp_str, "5:nodes", 7 );
    tmp_str += 7;

    int nearest_node_num = 0;

///////////////////////////////////////////
    bucket_tree_node_t *nearest_node = NULL;
    int ret = find_prop_node(node->bkt_tree, &nearest_node, (u_8 *)info_hash);
    if (-1 == ret)
    {
        dht_print("[ERROR] not find tgt node bkt!\n");
        return;
    }
    char nodes_buffer[500] = {0};
    char *tmp_nodes_buf = nodes_buffer;
    for (int j = 0; j < BUCKET_SIZE; ++j)
    {
        if (nearest_node->peer_nodes[j].status != IN_USE)
        {
            continue;
        }
        memcpy(tmp_nodes_buf, nearest_node->peer_nodes[j].node_str, NODE_STR_LEN);
        tmp_nodes_buf += NODE_STR_LEN;

        memcpy(tmp_nodes_buf, nearest_node->peer_nodes[j].node_ip, NODE_STR_IP_LEN);
        tmp_nodes_buf += NODE_STR_PORT_LEN;

        memcpy(tmp_nodes_buf, nearest_node->peer_nodes[j].node_port, NODE_STR_PORT_LEN);

        nearest_node_num ++;
    }
    int nod_buf_len = nearest_node_num * (NODE_STR_LEN+NODE_STR_IP_LEN+ NODE_STR_PORT_LEN);

    char node_num_str[4] = {0};
    sprintf(node_num_str, "%d:", nod_buf_len);
    dht_print("get nearest (%s) peers!\n", node_num_str);
    memcpy(tmp_str, node_num_str, strlen(node_num_str) );
    tmp_str += strlen(node_num_str);


    memcpy(tmp_str, nodes_buffer, nod_buf_len);
    tmp_str += nod_buf_len;

    memcpy(tmp_str, "e1:t2:", 6);
    tmp_str += 6;

    memcpy(tmp_str, peer_tid, peer_tid_len);
    tmp_str += peer_tid_len;

    memcpy(tmp_str, "1:y1:re", 7);
    tmp_str += 7;
// send q------
    msg_t *m = (msg_t*)malloc(sizeof(msg_t));
    m->addr = *rcv_addr;
    m->buf_len = (tmp_str - rsp_str);
    if (m->buf_len > 500)
    {
        dht_print("[WARNING] msg too long (%d)\n", m->buf_len);
        free(m);
        m = NULL;
        return;
    }
    memcpy(m->buf, rsp_str, m->buf_len);

    if (q_mgr.snd_q.msg_cnt >= MAX_MSG_QUEUE_SIZE)
    {
        free(m);
        m = NULL;
        dht_print("[warning] snd_q is full\n");
        return;
    }


    u_32 frm_ip = (rcv_addr->sin_addr.S_un.S_addr);
    char out[20] ={0};
    inet_bin_to_string(frm_ip, out);

    dht_print("sending (%s) a on find peer msg:(%s)\n",out, rsp_str);

}

void handle_ping_rsp(ben_dict_t *dict, node_t *node)
{
    //printf("get a ping rsp...\n");
    str_ele_t *e = dict->e[0].p.dict_val_ref;
    int find_id = 0;
    char tid_str[8] = {0};
    u_32 tid = 0;
    int find_tid = 0;
    while (NULL != e)
    {
        //if (0 == strcmp(e->str, "r"))
        if (e->str[0] == 'r' || e->str[0] == 'a')
        {
            //printf("\nget ping rsp r: %s\n", e->p.dict_val_ref->str);
            str_ele_t *e1 = e->p.dict_val_ref->p.dict_val_ref;
            while (NULL != e1)
            {
                if (0 == strcmp(e1->str, "id"))
                {
                    dht_print("get ping rsp id of r: %s\n", e1->p.dict_val_ref->str);
                    find_id = 1;
                    break;
                }
                e1 = e1->p.list_next_ref;
            }

        }
        else if ('t' == e->str[0])
        {
            memcpy(tid_str, ((char*)e->p.dict_val_ref->str + 1), (e->p.dict_val_ref->str_len -1 ) );
            tid = (u_32)atol(tid_str);
            //dht_print("a ping rsp tid: (%s), num (%u)\n", tid_str, tid);
            find_tid = 1;
            break;
        }
       // printf("key:(%s)\n", e->str);
        e = e->p.list_next_ref;
    }
    if (!find_tid)
    {
        printf("not find tid value!\n");
        return;
    }

    map_entry *r = map_del(node->msg_ctx_map, (void*)&tid );
    if (NULL !=r)
    {
        refresh_route_ctx_t *ctx = (refresh_route_ctx_t*)r->val;
        //ctx->route_info->refresh_count_down -= 1;
        if (NULL ==ctx->route_info)
            return;
        ctx->route_info->status = IN_USE;
        ctx->route_info->refresh_times = PEER_DETECT_UNOK_PERIOD;
        ctx->route_info->refresh_count_down = PEER_REFRESH_INTERVAL;
    }
}

int handle_find_node_rsp(ben_dict_t *dict, node_t *node)
{
    char tid_str[8] = {0};
    u_32 tid = 0;
    int find_tid = 0;
    u_8 id[NODE_STR_LEN] = {0};
    u_8 nodes_str[500] = {0};
    int nodes_str_len = 0;
    int find_node_str = 0;
    str_ele_t *e = dict->e[0].p.dict_val_ref;
    while (NULL != e)
    {
        if (0 == strcmp(e->str, "r"))
        {
            //dht_print("\nget find node rsp r: %s\n", e->p.dict_val_ref->str);
            str_ele_t *e1 = e->p.dict_val_ref->p.dict_val_ref;
            while (NULL != e1)
            {
                //dht_print("\nget find node rsp id of r: %s\n", e1->str);
                if (0 == strcmp(e1->str, "id"))
                {
                    //dht_print("\nget find node rsp id of r: %s\n", e1->p.dict_val_ref->str);
                    memcpy(id, e1->p.dict_val_ref->str, 20*sizeof(u_8));
                }
                if (0 == strcmp(e1->str, "nodes"))
                {
                    if (NULL == e1->p.dict_val_ref)
                    {
                        return -1;
                    }
                    nodes_str_len = e1->p.dict_val_ref->str_len;
                    if (0 == nodes_str_len || NULL == e1->p.dict_val_ref->str)
                    {
                        return -1;
                    }
                    //dht_print("\nget ping rsp nodes of r(len:%d): %s\n", nodes_str_len, e1->p.dict_val_ref->str);
                    memcpy(nodes_str, e1->p.dict_val_ref->str, nodes_str_len*sizeof(u_8));
                    //return 0;
                    find_node_str = 1;
                }

                e1 = e1->p.list_next_ref;
            }

        }
        else if ('t' == e->str[0])
        {
            memcpy(tid_str, ((char*)e->p.dict_val_ref->str + 1), (e->p.dict_val_ref->str_len -1 ) );
            tid = (u_32)atol(tid_str);
            //dht_print("a ping rsp tid: (%s), num (%u)\n", tid_str, tid);
            find_tid = 1;
        }
        e = e->p.list_next_ref;
    }
    if (0 == find_node_str)
    {
       dht_print("not find key r !\n");
        return -1;
    }

    if (!find_tid)
    {
        printf("not find tid value --- find node rsp !\n");
        return -1;
    }

    map_entry *r = map_del(node->msg_ctx_map, (void*)&tid );
    if (NULL !=r)
    {
        refresh_route_ctx_t *ctx = (refresh_route_ctx_t*)r->val;
        //ctx->route_info->refresh_count_down -= 1;
        if (ctx->route_info != NULL)
        {
            ctx->route_info->status = IN_USE;
            ctx->route_info->refresh_times = PEER_DETECT_UNOK_PERIOD;
            ctx->route_info->refresh_count_down = PEER_REFRESH_INTERVAL;
        }
    }


    int node_ip_port_len = (NODE_STR_LEN + NODE_STR_IP_LEN + NODE_STR_PORT_LEN);
    int node_num = nodes_str_len / node_ip_port_len;
    //dht_print("nodes_len (%d), num (%d)\n", nodes_str_len, node_num);
    if (node_num *node_ip_port_len != nodes_str_len )
    {
        dht_print("nodes string is not compelete..\n");
        return -1;
    }

    u_8 * temp =(u_8*)nodes_str;
    int ret = -1;
    /*for (int j = 0; j < node_num ; ++j )
    {
        for(int k=0;k< 26; ++k)
        {
            dht_print("%hhu.", *(temp+j*26+k) );
        }
        dht_print("\n");
    }*/
    int i;
    for (i= 0; i < node_num; ++i)
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
    //dht_print("insert (%d) record to memory!\n now save to file\n", i);
    //if (node_num > 0)
    //save_route_tbl_to_file(node);
    return 0;

}
int rcv_msg(node_t*node)
{
    SOCKET s = node->recv_socket;
    struct sockaddr_in in_add;
    int in_add_len = sizeof(struct sockaddr_in);
    int buffer_len = 500;
    char buffer[500] = {0};
    int ret = recvfrom(s,buffer,buffer_len,0,(struct sockaddr*)&in_add,&in_add_len);
    u_32 frm_ip = (in_add.sin_addr.S_un.S_addr);
    char out[20] ={0};
    inet_bin_to_string(frm_ip, out);
    if (ret > 0)
    {
        //dht_print("rcv msg frm-----(%s:%u)---------,len(%d).\n",out,in_add.sin_port, ret);
        msg_t *m = (msg_t*)malloc(sizeof(msg_t));
        m->msg_type = RECV_MSG;
        m->addr = in_add;
        m->buf_len = ret;
        memcpy(m->buf, buffer, ret);
        handle_rcv_msg(m,node);
        return OK;
    }
    else
    {
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
    dht_print("now:%u\n", t);
    sprintf(time_str,"%u", t);
    u_32 _hash [5] = {0};
    sha_1((u_char*)time_str,strlen(time_str), _hash);
    dht_print("random self id hash:");
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
        dht_print("err no:%d\n",errno);
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
        dht_print("new socket err\n");
        return -1;
    }
    u_long mode = 1;//set none-block socket
    int r_ctl = ioctlsocket(node->recv_socket,FIONBIO,&mode);
    if (r_ctl != 0)
    {
        dht_print("ioctl err:%d\n", r_ctl);
        closesocket(node->recv_socket);
        return -1;
    }
    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(6881);
    local_addr.sin_addr.S_un.S_addr = INADDR_ANY;// inet_addr("127.0.0.1") ;//INADDR_ANY;

    int ret = bind(node->recv_socket,(struct sockaddr*)&local_addr, sizeof(struct sockaddr));
    if (0 != ret)
    {
        dht_print("bind err:%d\n", ret);
        closesocket(node->recv_socket);
        return -1;
    }

    return 0;
}

void clear_node(node_t *node)
{
    WSACleanup();
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
    ret = init_self_node_id(node);
    node->msg_ctx_map = (hash_tbl*)calloc(1, sizeof(hash_tbl));
    node->torrent_hash_map = (hash_tbl*)calloc(1, sizeof(hash_tbl));
    init_msg_ctx_map(node->msg_ctx_map);
    init_torrent_hash_map(node->torrent_hash_map);
    return 0;
}

void convert_node_str(u_8 *src, u_8 * dst)
{
    memcpy(dst+1, src, NODE_STR_LEN);
}

int is_node_leaf(bucket_tree_node_t*bkt_tree)
{
    if (bkt_tree == NULL)
    {
        dht_print("what the fuck!?\n");
    }
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
    int idx = bits / 8;
    int rest = bits % 8;
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
    //print_node_str(_add, NODE_STR_LEN +1);
    big_bit_half(_add, ret);
}

int __check_if_split_ok(bucket_tree_node_t*bkt_tree, bucket_tree_node_t*parent_tree)
{
    u_8 tmp_middle_str[NODE_STR_LEN +1] ={0};
    generate_middle_str(tmp_middle_str, bkt_tree->range_start_str, bkt_tree->range_end_str);

    memcpy(bkt_tree->l->range_start_str, bkt_tree->range_start_str, NODE_STR_LEN +1);
    memcpy(bkt_tree->l->range_end_str, tmp_middle_str, NODE_STR_LEN +1);

    memcpy(bkt_tree->r->range_start_str, tmp_middle_str, NODE_STR_LEN+1);
    memcpy(bkt_tree->r->range_end_str, bkt_tree->range_end_str, NODE_STR_LEN+1);

    memcpy(bkt_tree->range_start_str, tmp_middle_str, NODE_STR_LEN+1);
    memcpy(bkt_tree->range_end_str, tmp_middle_str, NODE_STR_LEN+1);

    int ll = 0;
    int rr = 0;
    for (int j = 0; j < BUCKET_SIZE; ++j)
    {
        u_8 t[NODE_STR_LEN+1] = {0};
        convert_node_str(parent_tree->peer_nodes[j].node_str, t);
        if (1 == cmp_node_str(tmp_middle_str, t))
        {
            ll ++;
        }
        else
        {
            rr ++;
        }
    }

    if (0 == ll )
    {
        return 1;
    }
    if (0 == rr)
    {
        return -1;
    }

    return 0;

}


int __split_into_two_bkt(bucket_tree_node_t*parent)
{
    parent->l = (bucket_tree_node_t*)malloc(sizeof(bucket_tree_node_t));
    parent->r = (bucket_tree_node_t*)malloc(sizeof(bucket_tree_node_t));
    if (NULL == parent->l || NULL == parent->r)
    {
        free(parent->l);
        free(parent->r);
        parent->l = NULL;
        parent->r = NULL;
        dht_print("[debug] mem alloc err\n");
        return -1;
    }
    memset(parent->l, 0, sizeof(bucket_tree_node_t));
    memset(parent->r, 0, sizeof(bucket_tree_node_t));
    return 0;

}
/**
* here node_str is NODE_STR_LEN +1
*/
int insert_into_bkt(node_t *node, bucket_tree_node_t *bkt_tree, u_8 *node_str, u_8 *node_ip, u_8 *node_port, u_8 *self_node_id)
{
    int include_self = 0;
    int first_empty_slot = -1;
    for (int i = 0; i < BUCKET_SIZE; ++ i)
    {
        u_8 t[NODE_STR_LEN+1] = {0};
        convert_node_str(bkt_tree->peer_nodes[i].node_str, t);

        if (0 == cmp_node_str(node_str, t )) //same node_str
        {
            if (bkt_tree->peer_nodes[i].status == NOT_USE)
            {
                //dht_print("[ERROR] should'nt happen \n");
                bkt_tree->peer_nodes[i].status == IN_USE;
                //bkt_tree->peer_nodes[i].status = IN_USE;
            }
            //dht_print("@@@@@@@@@@@@@@@@@@@@@@ same node: %d\n", node->route_num);

            //print_node_str(t, NODE_STR_LEN +1);
            //print_node_str(node_str, NODE_STR_LEN +1);


            return 0 ;
        }
        if (bkt_tree->peer_nodes[i].status == NOT_USE && -1 == first_empty_slot )
        {
            first_empty_slot = i;
            break;
        }

        /*if (0 == cmp_node_str(self_node_id, bkt_tree->peer_nodes[i].node_str))
        {
            include_self = 1;
            dht_print("[debug] include self 1\n");
        }*/
    }

    if (-1 != first_empty_slot )
    {
        bkt_tree->peer_nodes[first_empty_slot].status = IN_USE;
        memcpy(bkt_tree->peer_nodes[first_empty_slot].node_str, node_str +1 , NODE_STR_LEN);
        memcpy(bkt_tree->peer_nodes[first_empty_slot].node_ip, node_ip, NODE_STR_IP_LEN);
        memcpy(bkt_tree->peer_nodes[first_empty_slot].node_port, node_port, NODE_STR_PORT_LEN);
        bkt_tree->peer_nodes[first_empty_slot].refresh_count_down = PEER_REFRESH_INTERVAL;
        bkt_tree->peer_nodes[first_empty_slot].refresh_times = PEER_DETECT_UNOK_PERIOD;
        //time_t t ;
        //time(&t);
        //bkt_tree->peer_nodes[first_empty_slot].update_time = t;
        node->route_num += 1;
        //dht_print("[debug] insert one to NOT_USE slot(%d)\n", first_empty_slot);
        //print_node_str(node_str, NODE_STR_LEN+1);
        return 0;
    }

    u_8 _self_id[NODE_STR_LEN+1] = {0};
    convert_node_str(self_node_id, _self_id);

    if (!(cmp_node_str(bkt_tree->range_start_str, _self_id) <= 0 && cmp_node_str(bkt_tree->range_end_str, _self_id) > 0))
    {
        //print_node_str(_self_id, NODE_STR_LEN+1);
        //dht_print("okkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk\n");
        //print_node_str(bkt_tree->range_start_str, NODE_STR_LEN+1);
        //print_node_str(_self_id, NODE_STR_LEN+1);
        //print_node_str(bkt_tree->range_end_str, NODE_STR_LEN+1);
        //dht_print("opppppppppppppppppppppppppppppppppppppppp\n");
        return 0;
    }
    // bkt should split into two bucket
    //dht_print("ssssssssssssplit~~~~\n");
    int split_ok = 0;
    bucket_tree_node_t *tmp_root = bkt_tree;
    while(!split_ok)
    {
        __split_into_two_bkt(tmp_root);
        int ret = __check_if_split_ok(tmp_root, bkt_tree);
        if (1 == ret)
        {
            tmp_root = tmp_root->r;
        }
        else if (-1 == ret)
        {
            tmp_root = tmp_root->l;
        }
        else
        {
            split_ok = 1;
        }
    }


    int ll = 0;
    int rr = 0;
    for (int j = 0; j < BUCKET_SIZE; ++j)
    {
        u_8 t[NODE_STR_LEN+1] = {0};
        convert_node_str(bkt_tree->peer_nodes[j].node_str, t);
        if (1 == cmp_node_str(tmp_root->range_start_str, t))
        {
            memcpy(&tmp_root->l->peer_nodes[ll++], &bkt_tree->peer_nodes[j], sizeof(peer_info_t));
            //dht_print("[debug] re-insert one to NOT_USE slot LEFT %d,%d \n", tmp_root->l->l == NULL, tmp_root->l->r == NULL);
        }
        else
        {
            memcpy(&tmp_root->r->peer_nodes[rr++], &bkt_tree->peer_nodes[j], sizeof(peer_info_t));
            //dht_print("[debug] re-insert one to NOT_USE slot RIGHT %d,%d !\n", tmp_root->r->l == NULL, tmp_root->r->r == NULL);
        }
    }
    //insert cur new node_str
    if (1 == cmp_node_str(tmp_root->range_start_str, node_str))
    {
        memcpy(tmp_root->l->peer_nodes[ll].node_str, node_str +1, NODE_STR_LEN);
        memcpy(tmp_root->l->peer_nodes[ll].node_ip, node_ip, NODE_STR_IP_LEN);
        memcpy(tmp_root->l->peer_nodes[ll].node_port, node_port, NODE_STR_PORT_LEN);
        tmp_root->l->peer_nodes[ll].status = IN_USE;
        tmp_root->l->peer_nodes[ll].refresh_count_down = PEER_REFRESH_INTERVAL;
        tmp_root->l->peer_nodes[ll].refresh_times = PEER_DETECT_UNOK_PERIOD;
        /*time_t t ;
        time(&t);
        tmp_root->r->peer_nodes[ll].update_time = t;*/
        //dht_print("[debug] re-insert one to NOT_USE slot LEFT NEW %d,%d !\n", tmp_root->l->l == NULL, tmp_root->l->r == NULL);
    }
    else
    {
        memcpy(tmp_root->r->peer_nodes[rr].node_str, node_str +1, NODE_STR_LEN);
        memcpy(tmp_root->r->peer_nodes[rr].node_ip, node_ip, NODE_STR_IP_LEN);
        memcpy(tmp_root->r->peer_nodes[rr].node_port, node_port, NODE_STR_PORT_LEN);
        tmp_root->r->peer_nodes[rr].status = IN_USE;
        tmp_root->r->peer_nodes[rr].refresh_count_down = PEER_REFRESH_INTERVAL;
        tmp_root->r->peer_nodes[rr].refresh_times = PEER_DETECT_UNOK_PERIOD;
        /*time_t t ;
        time(&t);
        tmp_root->r->peer_nodes[rr].update_time = t;*/
        //dht_print("[debug] re-insert one to NOT_USE slot RIGHT NEW %d,%d !\n", tmp_root->r->l == NULL, tmp_root->r->r == NULL);
    }
    node->route_num += 1;
    //dht_print("init okkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk\n");
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
            dht_print("malloc err\n");
            return -1;
        }
        memset((*bkt_tree), 0, sizeof(bucket_tree_node_t));
        memcpy((*bkt_tree)->peer_nodes[0].node_str, node_str, NODE_STR_LEN * sizeof(char));
        memcpy((*bkt_tree)->peer_nodes[0].node_ip, node_ip, sizeof(u_32));
        memcpy((*bkt_tree)->peer_nodes[0].node_port, node_port, sizeof(u_16));
        (*bkt_tree)->peer_nodes[0].status = IN_USE;
        (*bkt_tree)->peer_nodes[0].refresh_count_down = PEER_REFRESH_INTERVAL;
        (*bkt_tree)->peer_nodes[0].refresh_times = PEER_DETECT_UNOK_PERIOD;
        /*time_t t ;
        time(&t);
        (*bkt_tree)->peer_nodes[0].update_time = t;*/

        u_8 start_str[NODE_STR_LEN +1 ] = {0};
        generate_str_by_bit(0, start_str);
        memcpy((*bkt_tree)->range_start_str, start_str, NODE_STR_LEN +1 );
        //print_node_str(start_str, NODE_STR_LEN+1);

        u_8 end_str[NODE_STR_LEN +1 ] = {0};
        generate_str_by_bit(160, end_str);
        //print_node_str(end_str, NODE_STR_LEN+1);
        memcpy((*bkt_tree)->range_end_str, end_str, NODE_STR_LEN +1 );

        (*bkt_tree)->l = NULL;
        (*bkt_tree)->r = NULL;
        node->route_num += 1;

        //dht_print("[debug] insert one to mem.\n");

        u_8 dst1[NODE_STR_LEN+1] = {0};
        convert_node_str(node_str, dst1);

        //print_node_str(dst1, NODE_STR_LEN+1);

        return 0;

    }

    bucket_tree_node_t *the_node = NULL;
    u_8 dst[NODE_STR_LEN+1] = {0};
    convert_node_str(node_str, dst);
    int ret = find_prop_node(*bkt_tree, &the_node, dst);
    if (-1 == ret)
    {
        dht_print("Not find !! must be err\n");
        return -1;
    }

    ret = insert_into_bkt(node, the_node, dst, node_ip, node_port, self_node_id);
    //dht_print("----insert ip : %hhu.%hhu.%hhu.%hhu, port: %hu\n", node_ip[0],node_ip[1],node_ip[2],node_ip[3], *node_port);
    if (-1 == ret)
    {
        dht_print("Insert into bkt failed\n");
        return -1;
    }

    return 0;
}

int save_route_tbl_to_file(node_t* node)
{
    //dht_print("fush route to file ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    FILE *fp = NULL;
    fp = fopen("dht_route_s.dat", "wb+");
    if (NULL == fp)
    {
        dht_print("open write file failed\n");
        return -1;
    }

    tree_stack_t t_stack;
    tree_stack_t *t=&t_stack;
    init_tree_stack(t);

    bucket_tree_node_t *root = node->bkt_tree;
    //dht_print("root is null ? %d\n", root ==NULL);
    if (NULL == root)
        return -1;

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
                    u_16 _port = *(u_16*)(tmp->peer_nodes[i].node_port);
                    u_16 _port1 = dht_ntohs(tmp->peer_nodes[i].node_port);
                    //u_8 *_ip = tmp->peer_nodes[i].node_ip;
                    u_32 _ip = *(u_32*)tmp->peer_nodes[i].node_ip;
                    //dht_print("write ip: %hhu.%hhu.%hhu.%hhu, port :%hu, %hu\n ",_ip[0], _ip[1], _ip[2], _ip[3] , _port1, _port);
                    char out[20]={0};
                    inet_bin_to_string(_ip, out);
                    //dht_print("write ip: %s, %hu \n", out, _port1);
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
        dht_print("open read file failed\n");
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    if (0 == file_size)
    {
        dht_print("file is empty!\n");
        fclose(fp);
        return 0;
    }
    //read self node id
    //unsigned char self_node_id[NODE_STR_LEN] = {0};
    fseek(fp, 0, SEEK_SET);
    size_t read_len = fread(node->node_id, 1, NODE_STR_LEN, fp);
    if (read_len != NODE_STR_LEN)
    {
        dht_print("read self node id err(%d)\n", read_len);
        fclose(fp);
        return -1;
    }
    dht_print("use my node id in config:");
    print_hex(node->node_id, 20);

    //read route num
    int route_num = 0;
    read_len = fread(&route_num,  1, sizeof(int), fp);
    if (read_len != sizeof(int))
    {
        dht_print("read route num err(%u)\n", read_len);
        fclose(fp);
        return -1;
    }

    dht_print("read route num : %d\n", route_num);
    //node->route_num = route_num;

    //read route tbl
    u_8 * tmp_node_str = NULL;
    int str_len = (NODE_STR_LEN + NODE_STR_IP_LEN + NODE_STR_PORT_LEN);
    size_t buf_size = str_len * sizeof(unsigned char) * route_num;
    tmp_node_str = (u_8*)malloc(buf_size);
    if (NULL == tmp_node_str)
    {
        fclose(fp);
        dht_print("malloc mem (size: %u) err\n", buf_size);
        return -1;
    }

    read_len = 0;
    read_len = fread(tmp_node_str, str_len * sizeof(unsigned char), route_num, fp);
    if (read_len != route_num)
    {
        dht_print("[warning] read err,real node cnt : %d\n", read_len);
        //free(tmp_node_str);
        //fclose(fp);
        //return -1;

        dht_print("change route num frm (%u) to (%u)", route_num, read_len);
        route_num = read_len;
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
        dht_print("[ERROR] not equal! read from file (%d), inserted route_num (%d)\n", route_num, node->route_num);
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
        dht_print("read route success.\n");
        return 0;
    }
    dht_print("start init route table by find node!\n");
    //ping bootstrap node
    struct sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(6881);
    //remote_addr.sin_port = htons(49246);
    remote_addr.sin_addr.S_un.S_addr = (inet_addr("67.215.246.10"));
    //remote_addr.sin_addr.S_un.S_addr = (inet_addr("82.221.103.244"));
    //remote_addr.sin_addr.S_un.S_addr = (inet_addr("223.87.179.156"));
    //remote_addr.sin_addr.S_un.S_addr = (inet_addr("87.98.162.88"));
    ret = find_node(node,&remote_addr,node->node_id, node->node_id,NULL);
    ret = ping_node(node, &remote_addr,node->node_id, NULL);
     int try_times = 0;
     while (try_times -- > 0)
     {
         dht_print("rcv msg times %d:\n", try_times);
         ret = rcv_msg(node);
         if (ret == OK)
         {
             break;
         }
         //Sleep(200);
     }

    /**
     * add ret to except node list
     *
     * */
     return ret;
}

void _send_first_msg(node_t*nod)
{
    //list_head_t tmp_node = {&tmp_node,&tmp_node};
    //list_replace(&tmp_node, q_mgr.snd_q.q.node);
    int msg_cnt = q_mgr.snd_q.msg_cnt;
    while(q_mgr.snd_q.msg_cnt > 0 )
    {
        list_head_t *first = q_mgr.snd_q.q.node.next;
        msg_t *m = container_of(msg_t, node, first);

        sendto(nod->recv_socket,(char *)m->buf, m->buf_len, 0,(struct sockaddr*)&m->addr, sizeof(struct sockaddr));

        list_del(first);

        q_mgr.snd_q.msg_cnt -= 1;
        char out[21] ={0};
        inet_bin_to_string(m->addr.sin_addr.S_un.S_addr, out);
        //dht_print("send one msg {%s} len {%d} to =====(%s)=====, snd q len (%d)\n",m->buf, m->buf_len,out, q_mgr.snd_q.msg_cnt);

        free(m);
        m = NULL;
    }
    dht_print("send (%d) msgs.\n", msg_cnt);
}

/**
*
*/
int tree_node_to_buffer(node_t *node, route_and_peer_addr_in_mem_t *buffer, u_32 *real_route_num)
{
    if (NULL == buffer)
    {
        dht_print("malloc buf err\n");
        return -1;
    }
    route_and_peer_addr_in_mem_t *buf = buffer;

    tree_stack_t t_stack;
    tree_stack_t *t=&t_stack;
    init_tree_stack(t);

    bucket_tree_node_t *root = node->bkt_tree;
    //dht_print("root is null? ? %d\n", root ==NULL);
    if (NULL == root)
    {
        return -1;
    }

    tree_push(t, &root);
    u_32 r_num = 0;
    u_32 in_use_num = 0;

    while(!tree_stack_is_empty(t))
    {
        bucket_tree_node_t *tmp = NULL;
        if (-1 == tree_pop(t, &tmp))
            continue;

        if (is_node_leaf(tmp))
        {
            for (int i = 0; i < BUCKET_SIZE; ++i)
            {

                if ( (tmp->peer_nodes[i].status == IN_USE && tmp->peer_nodes[i].refresh_count_down >0) )
                {
                    memcpy(buf->node_str, tmp->peer_nodes[i].node_str, NODE_STR_LEN);
                    memcpy(buf->node_ip, tmp->peer_nodes[i].node_ip, NODE_STR_IP_LEN);
                    memcpy(buf->node_port, tmp->peer_nodes[i].node_port, NODE_STR_PORT_LEN);
                    buf->peer_addr_in_mem = &tmp->peer_nodes[i];
                    //dht_print("gggggggggg %p ggggggg\n", buf->peer_addr_in_mem);
                    ++ buf;
                    ++ r_num;

                    //u_32 _ip =*(u_32*)tmp->peer_nodes[i].node_ip;
                    //dht_print("buffer ip: %hhu.%hhu.%hhu.%hhu, port :%hu\n ",_ip[0], _ip[1], _ip[2], _ip[3] , _port);

                   // u_16 _port1 = dht_ntohs(tmp->peer_nodes[i].node_port);

                    //char out[20]={0};
                    //inet_bin_to_string(_ip, out);
                    //dht_print("buffer ip: %s, %hu \n", out, _port1);

                }

                if (tmp->peer_nodes[i].status == IN_USE)
                {
                    ++ in_use_num;
                    tmp->peer_nodes[i].refresh_count_down -= 1;
                    if (0 == tmp->peer_nodes[i].refresh_count_down)
                    {
                        tmp->peer_nodes[i].refresh_count_down = PEER_REFRESH_INTERVAL;
                        //dht_print("@@@@@@@@@@@@@@@@@@@@@@@@@ count down for a period (%d) @@@@@@\n", tmp->peer_nodes[i].refresh_count_down);
                        //dht_print("@@@@@@@@@@@@@@@@@@@@@@@@@ count down for a period times (%d) @@@@@@\n", tmp->peer_nodes[i].refresh_times);
                        tmp->peer_nodes[i].refresh_times -= 1;
                        if (0 == tmp->peer_nodes[i].refresh_times)
                        {
                            tmp->peer_nodes[i].status = NOT_USE;
                        }
                    }

                }

            }
        }
        else
        {
            if (tmp->l != NULL)
            tree_push(t, &tmp->l);
            if (tmp->r != NULL)
            tree_push(t, &tmp->r);
        }
    }

    *real_route_num = r_num;
    //dht_print("[INFO] ......... in use num (%u) .........\n", in_use_num);
    return 0;
}

void refresh_route(node_t *node, int type)
{
    int route_num = node->route_num;
    route_and_peer_addr_in_mem_t *buffer = (route_and_peer_addr_in_mem_t*)malloc(sizeof(route_and_peer_addr_in_mem_t) * route_num);
    if (NULL == buffer)
    {
        dht_print("malloc buf err\n");
        return ;
    }
    u_32 real_route_num = 0;
    int ret = tree_node_to_buffer(node, buffer, &real_route_num);
    dht_print("****************cur route num: %d, real num (%u)\n", route_num, real_route_num);

    struct sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(6881);
    remote_addr.sin_addr.S_un.S_addr = (inet_addr("67.215.246.10"));


    route_and_peer_addr_in_mem_t *tmp = buffer;
    for (int i = 0; i < real_route_num; ++i)
    {
        u_32* ip =(u_32*)(buffer[i].node_ip);
        remote_addr.sin_addr.S_un.S_addr = *ip;

        u_16 * port = (u_16*)(buffer[i].node_port);
        remote_addr.sin_port = *port;

        if (type == Y_TYPE_PING)
        {
            ret = ping_node(node, &remote_addr, node->node_id, buffer[i].peer_addr_in_mem);
            //dht_print("######### %p #######\n", buffer[i].peer_addr_in_mem);
        }
        else if(type == Y_TYPE_FIND_NODE)
        {
            ret = find_node(node, &remote_addr,node->node_id, node->node_id, buffer[i].peer_addr_in_mem);
        }
    }

    free(buffer);
    buffer = NULL;
}

/**
sess = lt.session()
sess.add_dht_router('router.bittorrent.com', 6881)
sess.add_dht_router('router.utorrent.com', 6881)
sess.add_dht_router('router.bitcomet.com', 6881)
sess.add_dht_router('dht.transmissionbt.com', 6881)
sess.start_dht();
*/
void find_neighbor(node_t*node)
{
    if (node->route_num == 0)
    {
        struct sockaddr_in remote_addr;
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(6881);
        remote_addr.sin_addr.S_un.S_addr = (inet_addr("87.98.162.88"));
        //remote_addr.sin_addr.S_un.S_addr = (inet_addr("67.215.246.10"));
        find_node(node, &remote_addr,node->node_id, node->node_id, NULL);
        return;
    }
    refresh_route(node, Y_TYPE_FIND_NODE);
    //refresh_route(node, Y_TYPE_PING);
}

char tmp_buff[500] = {0};
int tmp_buff_len = 0;
u_32 pos = 0;
struct sockaddr_in rcv_frm_addr;

void handle_rcv_msg(msg_t *m, node_t *nod)
{
    memset(&dict, 0, sizeof(ben_dict_t));
    pos = 0;
    //dht_print("[info] buf:{%s} buflen{%d}\n", m->buf, m->buf_len);
    memset(tmp_buff, 0, 500);
    memcpy(tmp_buff, m->buf, m->buf_len);
    tmp_buff_len = m->buf_len;
    ben_coding(m->buf, m->buf_len, &pos);
    rcv_frm_addr = m->addr;

    q_mgr.rcv_q.msg_cnt -= 1;

    free(m);
    m = NULL;

    int rsp_type = -1;
    get_rsp_msg_type(&dict, &rsp_type);
    if (-1 == rsp_type)
    {
        dht_print("get rsp type failed [info] buf:{%s} buflen{%d}\n", tmp_buff, tmp_buff_len);
        dht_print("failed rcv buf (%s)\n", tmp_buff);
        return;
    }
    if (  rsp_type != Y_TYPE_PING_RSP &&  rsp_type != Y_TYPE_FIND_NODE_RSP)
    dht_print("handle rcv msg : type -> %s\n", type_desc[rsp_type]);
    switch(rsp_type)
    {
    case Y_TYPE_PING:
        handle_on_ping(&rcv_frm_addr, &dict, nod);
        break;
    case Y_TYPE_PING_RSP:
        handle_ping_rsp(&dict, nod);
        //dht_print("[info] buf:{%s} buflen{%d}\n", tmp_buff, tmp_buff_len);
        break;
    case Y_TYPE_FIND_NODE:
        handle_on_find_node(&rcv_frm_addr, &dict, nod);
        //dht_print("[info] buf:{%s} buflen{%d}\n", tmp_buff, tmp_buff_len);
        break;
    case Y_TYPE_FIND_NODE_RSP:
        handle_find_node_rsp(&dict, nod);
        break;
    case Y_TYPE_GET_PEER:
        handle_on_get_peer(&rcv_frm_addr, &dict, nod);
        //dht_print("[info] buf:{%s} buflen{%d}\n", tmp_buff, tmp_buff_len);
        break;
    case Y_TYPE_GET_PEER_RSP:
        //dht_print("[info] buf:{%s} buflen{%d}\n", tmp_buff, tmp_buff_len);
        break;
    case Y_TYPE_ANNOUNCE_PEER:
    case Y_TYPE_ANNOUNCE_PEER_RSP:
        dht_print("[info] buf:{%s} buflen{%d}\n", tmp_buff, tmp_buff_len);
        break;

    default:
        dht_print("[info] buf:{%s} buflen{%d}\n", tmp_buff, tmp_buff_len);
        break;

    }
}

/**
* timer for refresh route table
*/
void *timer_thread(void*arg)
{
    //Sleep(5000);
    unsigned long long tick = 0;
    node_t *nod = (node_t*)arg;
    int i = 0;
    while(1)
    {
        //dht_print("re-init (%d) time\n", ++i);
        /*sockaddr_in remote_addr;
        remote_addr.sin_family = AF_INET;
        u_16 x_port ;
        u_8* x_low = (u_8*)&x_port;
        *x_low = 0x1a;
        *(x_low+1) = 0xe1;
        remote_addr.sin_port = x_port;//htons(6881);
        remote_addr.sin_addr.S_un.S_addr = (inet_addr("67.215.246.10"));
        u_32 _ip ;
        u_8 *y_ip = (u_8*)&_ip;
        *y_ip = 87;
        *(y_ip+1) = 98;
        *(y_ip+2) = 162;
        *(y_ip+3) = 88;*/
        //remote_addr.sin_addr.S_un.S_addr = _ip;// (inet_addr("87.98.162.88"));
        //int ret = find_node(nod->recv_socket,&remote_addr,nod->node_id, nod->node_id);
        //int ret = ping_node(&remote_addr,nod->node_id);
        //find_neighbor(nod);
        map_entry *iter = NULL;
        map_for_each(timer_map, iter)
        {
            timer_arg_t *ta = (timer_arg_t*)iter->val;
            if (tick % ta->interval != 0)
            {
                continue;
            }
            msg_t *m = (msg_t*)malloc(sizeof(msg_t));
            if (NULL == m)
            {
                dht_print("[FATAL] can't malloc mem\n");
                break;
            }
            m->timer_id = ta->timer_id;
            m->msg_type = TIMEOUT_MSG;

            //dht_print("[DEBUG] a timer ,timer id (%d)\n", m->timer_id);
            if (q_mgr.rcv_q.msg_cnt >= MAX_MSG_QUEUE_SIZE)
            {
                free(m);
                m = NULL;
                dht_print("[WARNING] sending queue is full\n");
                break;
            }
        }

        ++ tick;
        Sleep(1000);
    }
}

/**
* process msg in msg que
*/
void *msg_schedule_thread(void*arg)
{
    node_t *nod = (node_t*)arg;
    map_entry *x = NULL;
    timer_arg_t * y = NULL;
    while(1)
    {


        while(q_mgr.rcv_q.msg_cnt <= 0)
        {

        }

        list_head_t *first = q_mgr.rcv_q.q.node.next;
        msg_t *m = container_of(msg_t, node, first);
        x = NULL;
        y = NULL;
        switch(m->msg_type)
        {
        case TIMEOUT_MSG:
            x = map_get(timer_map, (void*)&m->timer_id);
            if (NULL == x)
            {
                dht_print("[FATAL] should not happen!\n");
                break;
            }
            y = (timer_arg_t*)x->val;
            y->f(y->data);
            //dht_print("sending a timer msg\n");
            list_del(first);
            q_mgr.rcv_q.msg_cnt -= 1;

            free(m);
            m = NULL;

            break;
        case SEND_MSG:
            handle_rcv_msg(m, nod);
            break;
        case RECV_MSG:
            handle_rcv_msg(m, nod);
            break;
        default:
            break;
        }

    }

}

/**
* send/receive req msg(s), just use one thread to send and receive data
* block condition: receive_q is full and send_q is empty
* when other thread op receiv_q or send_q, then signal cur thread.
*/
void* net_thread(void*arg)
{
    node_t *nod = (node_t*)arg;
    while(1)
    {

        while (q_mgr.snd_q.msg_cnt <= 0 &&  q_mgr.rcv_q.msg_cnt >= MAX_MSG_QUEUE_SIZE)
        {

        }
;
        if (q_mgr.snd_q.msg_cnt > 0)
        {
            _send_first_msg(nod);
        }

        if (q_mgr.rcv_q.msg_cnt < MAX_MSG_QUEUE_SIZE)
        {
            //dht_print("rcv msg...\n");
            rcv_msg( nod);
            Sleep(50);
        }

    }
}

void init_torrent_hash_map(hash_tbl *m)
{
    map_init(m, str_hash, str_equal_f, 1 << 16, (1<<16) - 1);
}

void init_msg_ctx_map(hash_tbl *m)
{
    map_init(m, int_hash, int_equal_f, 1 << 16, (1<<16) - 1);
}

#define MAX_TIMEOUT_TID (10000)
void msg_timeout_handler(void*arg)
{
    node_t*node = (node_t*)arg;
    int timeout_msg_num = 0;
    u_32 tid_arr[MAX_TIMEOUT_TID] = {0};
    map_entry *e = NULL;
    map_for_each(node->msg_ctx_map, e)
    {
        u_32 tid = *(u_32*)(e->key);
        refresh_route_ctx_t *ctx = (refresh_route_ctx_t*)e->val;
        ctx->timeout_sec -= MSG_TIMEOUT_TIMER_INTERVAL;

        if (ctx->timeout_sec <= 0 && timeout_msg_num < MAX_TIMEOUT_TID)
        {
            tid_arr[timeout_msg_num ++] = tid;
            //dht_print("[ERROR] msg timeout, op_type (%s), tid (%u)\n", type_desc[ctx->op_type], tid);
        }
    }

    for (int i = 0; i < timeout_msg_num; ++i)
    {
        map_entry *del = map_del(node->msg_ctx_map, (void*)&tid_arr[i]);
    }

    e = NULL;
    int c = 0;
    map_for_each(node->msg_ctx_map, e)
    {
        if (e != NULL)
        {
            ++ c;
        }
    }
    if (timeout_msg_num > 0)
    {
        dht_print("del then left:(%d) delete (%d) time out ctx\n",c, timeout_msg_num);
    }
    //dht_print("ctx timeout timer running ......... \n");
}

void init_msg_timeout_timer(node_t*node)
{
    timer_arg_t *ta = (timer_arg_t*)malloc(sizeof(timer_arg_t));
    if (NULL == ta)
    {
        dht_print("[ERROR] malloc err\n");
        return;
    }
    ta->data = node;
    ta->f = msg_timeout_handler;//(node, Y_TYPE_PING);
    ta->timer_id = g_timer_id ++;
    ta->interval = MSG_TIMEOUT_TIMER_INTERVAL;

    map_entry e = {0};
    e.key = (void*)(&ta->timer_id);
    e.val = (void*)ta;
    map_put(timer_map, &e);
}
void init_timer_map(hash_tbl *m)
{
    map_init(m, int_hash, int_equal_f, 1<<16, (1<<16) -1 );
}

void period_ping(void*data)
{
    node_t *node = (node_t*)data;
    if (node->route_num == 0)
    {
        struct sockaddr_in remote_addr;
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(6881);
        remote_addr.sin_addr.S_un.S_addr = (inet_addr("87.98.162.88"));
        //remote_addr.sin_addr.S_un.S_addr = (inet_addr("67.215.246.10"));
        //find_node(node,&remote_addr,node->node_id, node->node_id, NULL);
        return;
    }

    refresh_route(node, Y_TYPE_PING);
    //refresh_route(node,Y_TYPE_FIND_NODE);
    map_entry *e  = NULL;
    map_for_each(torrent_hash_map, e)
    {
        print_info_hash((char*)e->key, 20);
    }

}

void init_ping_timer(node_t*node)
{
    timer_arg_t *ta = (timer_arg_t*)malloc(sizeof(timer_arg_t));
    if (NULL == ta)
    {
        dht_print("[ERROR] malloc err\n");
        return;
    }
    ta->data = node;
    ta->f = period_ping;//(node, Y_TYPE_PING);
    ta->timer_id = g_timer_id ++;
    ta->interval = 20;

    map_entry e = {0};
    e.key = (void*)(&ta->timer_id);
    e.val = (void*)ta;
    map_put(timer_map, &e);
}

int mainxyz()
{
    dht_print("ok dht spider\n");
    node_t node = {0};
    int ret = init_self_node(&node);
    if (-1 == ret)
    {
        clear_node(&node);
        return -1;
    }
    init_route_table(&node);
    g_req_id = 0;
    g_find_node_id = 0;
    //worker(&node);
    // update route table by ping node in route table

    // receive msg from other node.
    init_timer_map(timer_map);
    g_timer_id = 0;
    init_ping_timer(&node);

    //init_msg_ctx_map(msg_ctx_map);
    init_msg_timeout_timer(&node);

    //info hash map

    init_torrent_hash_map(torrent_hash_map);

    u_long tip = inet_addr("0.0.0.1");
    dht_print("inet addr (%u)\n", tip);
    u_8 *z = (u_8*)&tip;
    dht_print("low->high: (%hu.%hu.%hu.%hu)\n", *z, *(z+1),*(z+2), *(z+3));

    u_8 zz[]={1,2,3,4};
    tip = dht_ntohl(zz);
    z = (u_8*)&tip;
    dht_print("low->high: (%hu.%hu.%hu.%hu)\n", *z, *(z+1),*(z+2), *(z+3));

    u_8 zzz[]={4,5};
    tip = dht_ntohs(zzz);
    z = (u_8*)&tip;
    dht_print("low->high: (%hu.%hu)\n", *z, *(z+1));

    clear_node(&node);
/*
    dht_print("\ntest byte order of mingwin:\n");

    u_16 x = 0x1;
    u_8 *y = (u_8*)&x;

    int type = 0;
    dht_print("lower address of x is: %hu, higher address of x is %hu:\n", *y, *(y+1));


    char test_ping[]="d1:ad2:id20:abcdefghij0123456789e1:q4:ping1:t2:aa1:y1:qe";
    u_32 pos = 0;
    type = 0;
    memset(&dict, 0, sizeof(ben_dict_t));
    ben_coding(test_ping,strlen(test_ping),&pos);
    get_rsp_msg_type(&dict, &type);
    dht_print("get test_ping msg type  : %d -> %s\n", type, type_desc[type]);

    char test_ping_rsp[]="d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:pa1:y1:re";
    pos = 0;
    type = 0;
    memset(&dict, 0, sizeof(ben_dict_t));
    ben_coding(test_ping_rsp,strlen(test_ping),&pos);
    get_rsp_msg_type(&dict, &type);
    dht_print("get test_ping_rsp msg type : %d -> %s\n", type, type_desc[type]);


    char test_find_node_req[]="d1:ad2:id20:abcdefghij01234567896:target20:mnopqrstuvwxyz123456e1:q9:find_node1:t2:fa1:y1:qe";
    pos = 0;
    type = 0;
    memset(&dict, 0, sizeof(ben_dict_t));
    ben_coding(test_find_node_req,strlen(test_find_node_req),&pos);
    get_rsp_msg_type(&dict, &type);
    dht_print("get test_find_node_req msg type : %d -> %s\n", type, type_desc[type]);

    char test_find_node_rsp[]="d1:rd2:id20:0123456789abcdefghij5:nodes9:def456...e1:t2:fa1:y1:re";
    pos = 0;
    type = 0;
    memset(&dict, 0, sizeof(ben_dict_t));
    ben_coding(test_find_node_rsp,strlen(test_find_node_rsp),&pos);
    get_rsp_msg_type(&dict, &type);
    dht_print("get test_find_node_rsp msg type : %d -> %s\n", type, type_desc[type]);


    char test_get_peers_req[]="d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz123456e1:q9:get_peers1:t2:ga1:y1:qe";
    pos = 0;
    type = 0;
    memset(&dict, 0, sizeof(ben_dict_t));
    ben_coding(test_get_peers_req,strlen(test_get_peers_req),&pos);
    get_rsp_msg_type(&dict, &type);
    dht_print("get test_get_peers_req msg type : %d -> %s\n", type, type_desc[type]);

    char test_get_peers_rsp1[]="d1:rd2:id20:abcdefghij01234567895:token8:aoeusnth6:valuesl6:axje.u6:idhtnmee1:t2:ga1:y1:re";
    pos = 0;
    type = 0;
    memset(&dict, 0, sizeof(ben_dict_t));
    ben_coding(test_get_peers_rsp1,strlen(test_get_peers_rsp1),&pos);
    get_rsp_msg_type(&dict, &type);
    dht_print("get test_get_peers_rsp1 msg type : %d -> %s\n", type, type_desc[type]);

    char test_get_peers_rsp2[]="d1:rd2:id20:abcdefghij01234567895:nodes9:def456...5:token8:aoeusnthe1:t2:ga1:y1:re";
    pos = 0;
    type = 0;
    memset(&dict, 0, sizeof(ben_dict_t));
    ben_coding(test_get_peers_rsp2,strlen(test_get_peers_rsp2),&pos);
    get_rsp_msg_type(&dict, &type);
    dht_print("get test_get_peers_rsp2 msg type : %d -> %s\n", type, type_desc[type]);


    char test_announce_req[]="d1:ad2:id20:abcdefghij01234567899:info_hash20:mnopqrstuvwxyz1234564:porti6881e5:token8:aoeusnthe1:q13:announce_peer1:t2:aa1:y1:qe";
    pos = 0;
    type = 0;
    memset(&dict, 0, sizeof(ben_dict_t));
    ben_coding(test_announce_req,strlen(test_announce_req),&pos);
    get_rsp_msg_type(&dict, &type);
    dht_print("get test_announce_req msg type : %d -> %s\n", type, type_desc[type]);

    char test_announce_rsp[]="d1:rd2:id20:mnopqrstuvwxyz123456e1:t2:aa1:y1:re";
    pos = 0;
    type = 0;
    memset(&dict, 0, sizeof(ben_dict_t));
    ben_coding(test_announce_rsp,strlen(test_announce_rsp),&pos);
    get_rsp_msg_type(&dict, &type);
    dht_print("get test_announce_rsp msg type : %d -> %s\n", type, type_desc[type]);
*/

    return 0;
}
