//
// Created by xx on 2018/8/26.
//

#include <winsock.h>
#include <time.h>

#ifndef SHA1_DHT_SPIDER_H
#define SHA1_DHT_SPIDER_H

#endif //SHA1_DHT_SPIDER_H

typedef unsigned long long u_64;
typedef unsigned long u_32;

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