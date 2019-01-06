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
#define BUCKET_SIZE                  (8)
#define BUCKET_NUM                   (128)

enum OP_STATUS
{
    OK = 0, ERR = -1
};

enum BUCKET_STATUS
{
    IN_USE = 100, NOT_USE = 101
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
typedef struct node
{
    unsigned char node_id[NODE_STR_LEN];
    SOCKET recv_socket;
    SOCKET send_socket;
    bucket_t buckets[BUCKET_NUM];
    int buckets_num;
}node_t;

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
