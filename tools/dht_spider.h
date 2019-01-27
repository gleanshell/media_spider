//
// Created by xx on 2018/8/26.
//

#include <winsock.h>
#include <time.h>

#ifndef BT_PARSER_H

//#include "bt_parser.h"

#endif // BT_PARSER_H

#ifndef SHA1_DHT_SPIDER_H
#define SHA1_DHT_SPIDER_H

#endif //SHA1_DHT_SPIDER_H

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

enum OP_STATUS
{
    OK = 0, ERR = -1
};

enum BUCKET_STATUS
{
    NOT_USE = 0,
    IN_USE
};
typedef struct bucket
{
    unsigned char neibor_nodes[BUCKET_SIZE][NODE_STR_LEN];
    time_t update_time;
    int range_start;
    int range_end;
    int status;
    int nodes_num;
}bucket_t;


typedef struct peer_info
{
    u_8 node_str[NODE_STR_LEN];
    u_8 node_ip[NODE_STR_IP_LEN];
    u_8 node_port[NODE_STR_PORT_LEN];
    unsigned __int64 update_time;
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
    SOCKET send_socket;
    bucket_tree_node_t *bkt_tree;
    int buckets_num;
}node_t;

enum
{
    Y_PING_RSP = 1,
    Y_FIND_NODE_RSP
};

enum
{
    Y_TYPE_Q = 1, //request
    Y_TYPE_R,//response
    Y_TYPE_E // response err

};

typedef struct msg_ctx
{
    u_8 trans_id[2];
    int op_type;


}msg_ctx_t;
