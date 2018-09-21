//
// Created by xx on 2018/7/19.
//

#include <windef.h>
#include <winsock.h>
#include <cstdio>
#include "net.h"

#define PROTOCOL_NAME_LEN 19

extern void print_hex(unsigned char* n, u_32 len);


int main0000()
{
    WORD sock_ver = MAKEWORD(2,2);
    WSADATA wsadata ;
    if (WSAStartup(sock_ver,&wsadata))
    {
        printf("err\n");
        return errno;
    }
    SOCKET client = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if (INVALID_SOCKET == client)
    {
        printf("new socket err");
        return 0;
    }
    u_long mode = 0;
    int r_ctl = ioctlsocket(client,FIONBIO,&mode);
        sockaddr_in remote_addr;
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(6881);
        remote_addr.sin_addr.S_un.S_addr = (inet_addr("67.215.246.10"));
       // remote_addr.sin_addr.S_un.S_addr = (inet_addr("82.221.103.244"));

//        char buf[] ="d1:ad2:id20:abcdefghij01234567896:target20:abcdefghij0123456789e1:q9:find_node1:t2:aa1:y1:qe";
        char buf[] ="d1:ad2:id20:abcdefghij0123456789e1:q4:ping1:t2:aa1:y1:qe";
        int buf_len = strlen(buf);

        int r = sendto(client, buf,buf_len, 0, (sockaddr*)&remote_addr, sizeof(remote_addr));
        printf("rrrrr %d\n", r);

    char buffer[500] = {0};

//    int ret = recv(client,buffer,500,0);
        struct sockaddr_in in_add;
        int in_add_len = sizeof(struct sockaddr_in);
        int buffer_len = 500;
        int ret = recvfrom(client,buffer,buffer_len,0,(sockaddr*)&in_add,&in_add_len);
    if (ret > 0)
    {
        //buffer[ret] = 0;
        printf("%s\n\n",buffer);
        print_hex((unsigned char*)buffer,ret);
    }
    printf("ret %d\n", ret);
    int err = WSAGetLastError();
    printf("sock err:%d\n",err);

    closesocket(client);
    WSACleanup();
    return 0;
}