//
// Created by xx on 2018/8/26.
//

#ifndef SHA1_DHT_SPIDER_H
#define SHA1_DHT_SPIDER_H

//#include <winsock.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <time.h>
#include "list.h"
#include "hash_tbl.h"

typedef int SOCKET;
typedef unsigned long long u_64;
typedef unsigned long u_32;
typedef unsigned short int u_16;
typedef unsigned char u_8;

#define NODE_STR_LEN                 (20)
#define NODE_STR_IP_LEN                 (4)
#define NODE_STR_PORT_LEN                 (2)

#define BUCKET_SIZE                  (8)
#define BUCKET_NUM                   (128)

#ifndef MAX_STACK_SIZE

#define MAX_STACK_SIZE (500)

#endif // MAX_STACK_SIZE


#define PEER_REFRESH_INTERVAL (6) //seconds
#define PEER_DETECT_UNOK_PERIOD (5)
enum OP_STATUS
{
    OK = 0, ERR = -1
};

enum BUCKET_STATUS
{
    NOT_USE = 0,
    IN_USE
};
 enum MSG_CMD
{
    TIMEOUT_MSG = 1,
    SEND_MSG,
    RECV_MSG
};

typedef struct peer_info
{
    u_8 node_str[NODE_STR_LEN];
    u_8 node_ip[NODE_STR_IP_LEN];
    u_8 node_port[NODE_STR_PORT_LEN];
    int refresh_count_down; //PEER_REFRESH_INTERVAL=6
    int refresh_times; // PEER_DETECT_UNOK_PERIOD=5
    int status;
}peer_info_t;

typedef struct bucket_tree_node
{
    peer_info_t peer_nodes[BUCKET_SIZE];
    u_8 range_start_str[NODE_STR_LEN + 1];
    //int range_start;
    u_8 range_end_str[NODE_STR_LEN + 1];
    //int range_end;
    struct bucket_tree_node *l;
    struct bucket_tree_node *r;
}bucket_tree_node_t;

typedef struct tree_stack
{
    bucket_tree_node_t *stack_array[MAX_STACK_SIZE];
    int stack_size;
}tree_stack_t;

typedef struct node
{
    unsigned char node_id[NODE_STR_LEN];
    SOCKET recv_socket;
    bucket_tree_node_t *bkt_tree;
    int route_num;
    hash_tbl *msg_ctx_map;
    hash_tbl *torrent_hash_map;
}node_t;

enum
{
    Y_TYPE_BEGIN,
    Y_TYPE_Q = 1, //request
    Y_TYPE_R,//response
    Y_TYPE_E, // response err
    Y_TYPE_PING,
    Y_TYPE_PING_RSP,
    Y_TYPE_FIND_NODE,
    Y_TYPE_FIND_NODE_RSP,
    Y_TYPE_GET_PEER,
    Y_TYPE_GET_PEER_RSP,
    Y_TYPE_ANNOUNCE_PEER,
    Y_TYPE_ANNOUNCE_PEER_RSP,
    Y_TYPE_BUTT

};

typedef struct timer_arg
{
    int timer_id;
    int interval;// second
    void*data;
    void (*f)(void*);
} timer_arg_t;

typedef struct msg
{
    time_t t;
    char buf[500];
    int buf_len;
    struct sockaddr_in addr;
    int timer_id;
    enum MSG_CMD msg_type;
    u_64 tid;
    list_head_t node;
}msg_t;

#define MAX_MSG_QUEUE_SIZE (200000)
typedef struct msq_q
{
    msg_t q;
    int msg_cnt;
}msg_q_t;

typedef struct msg_q_mgr
{
    msg_q_t snd_q;
    msg_q_t rcv_q;
}msg_q_mgr_t;

typedef struct route_and_peer_addr_in_mem
{
    u_8 node_str[NODE_STR_LEN];
    u_8 node_ip[NODE_STR_IP_LEN];
    u_8 node_port[NODE_STR_PORT_LEN];
    peer_info_t *peer_addr_in_mem;
} route_and_peer_addr_in_mem_t;

#define MSG_TIMEOUT_INTERVAL (30)
#define MSG_TIMEOUT_TIMER_INTERVAL (1)
typedef struct refresh_route_ctx
{
    int op_type;//ping , get peer and so on
    int timeout_sec;
    peer_info_t *route_info;
}refresh_route_ctx_t;

int rcv_msg(node_t*node);
void period_ping(void*data);
void msg_timeout_handler(void*arg);
void find_neighbor(node_t*node);
int save_route_tbl_to_file(node_t* node);

#define dht_print(fmt,...)\
{\
dht_print_helper("[%s:%d]" fmt,__func__,__LINE__, ##__VA_ARGS__);\
}

#endif //SHA1_DHT_SPIDER_H
