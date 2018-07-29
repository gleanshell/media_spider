//
// Created by xx on 2018/7/27.
//

#ifndef SHA1_BT_PARSER_H
#define SHA1_BT_PARSER_H

#endif //SHA1_BT_PARSER_H

typedef unsigned long long u_64;
typedef unsigned long u_32;

#define MAX_URL_LEN 200
#define MAX_ANNOUNCE_NUM 200
#define MAX_COMMENT_LEN 200
#define MAX_FILE_NUM 200
#define MAX_FILE_PATH_LEN 200

typedef struct
{
    char path_name[MAX_FILE_PATH_LEN];
    u_32 length;
    char md5sum[32];
}single_file_t;

typedef struct
{
    u_32 file_num;
    single_file_t single_file[MAX_FILE_NUM];
}multi_file_t ;
typedef struct
{
    u_32 piece_len;
    char *sha1_pieces[20];
    union {
        single_file_t s;
        multi_file_t m;
    }u;
}meta_info_t;

typedef struct
{
    char announce[MAX_URL_LEN];
    u_32 announce_list_num;
    char announce_list [MAX_ANNOUNCE_NUM][MAX_URL_LEN];
    char create_date[12];
    char comment[MAX_COMMENT_LEN];
    char create_by[MAX_COMMENT_LEN];
    meta_info_t info;
}torrent_t;

#define MAX_STACK_SIZE 100

typedef struct
{
    char stack_array[MAX_STACK_SIZE];
    int stack_size;
} stack_t;

#define  WAIT_ANN              1
#define  WAIT_ANN_LIST         2
#define  WAIT_COMMT            3
#define  WAIT_CREATE_BY        4
#define  WAIT_CREATE_DATE      5
#define  WAIT_ENCODING         6
#define  WAIT_INFO_CONTENT             7
#define  WAIT_FILES_STR                8
#define  WAIT_FILES_VALUE              9
#define  WAIT_FILES_LEN_STR           10
#define  WAIT_FILES_LEN_VALUE         11
#define  WAIT_FILES_PATH_STR          12
#define  WAIT_FILES_PATH_VALUE        13

#define  WAIT_FILES_NAME       10
#define  WAIT_PIECE_LEN        11
#define  WAIT_PIECES           12
#define  WAIT_PUBLISHER        13
#define  WAIT_PUBLISHER_URL    14
#define  WAIT_NODES_IP         15
#define  WAIT_NODES_PORT       16

#define ELE_KEY                    '1'
#define ELE_VALUE                    '2'
#define ELE_LIST               '3'
#define ELE_INT               '4'
#define ELE_ANN_COMMT              '4'
#define ELE_CREATE_BY              '5'
#define ELE_CREATE_DATE            '6'
#define ELE_ENCODEING              '7'
