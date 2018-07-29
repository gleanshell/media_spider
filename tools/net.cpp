//
// Created by xx on 2018/7/19.
//

#include <windef.h>
#include <winsock.h>
#include <cstdio>
#include "net.h"

int main2()
{
    WORD sock_ver = MAKEWORD(2,2);
    WSADATA wsadata ;
    if (WSAStartup(sock_ver,&wsadata))
    {
        printf("err\n");
        return errno;
    }
    SOCKET client = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (INVALID_SOCKET == client)
    {
        printf("new socket err");
        return 0;
    }

    sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(443);
    remote_addr.sin_addr.S_un.S_addr = (inet_addr("202.108.35.250"));

    if (SOCKET_ERROR == connect(client, (sockaddr*)&remote_addr, sizeof(remote_addr)))
    {
        printf("connect err");
        return 0;
    }
    char send_data[] = "GET / HTTP/1.1\r\n"
                       "Host: www.baidu.com\r\n"
                       "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:61.0) Gecko/20100101 Firefox/61.0\r\n"
                       "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
                       "Accept-Language: zh-CN,zh;q=0.8,zh-TW;q=0.7,zh-HK;q=0.5,en-US;q=0.3,en;q=0.2\r\n"
                       "Accept-Encoding: gzip, deflate, br\r\n"
                       "DNT: 1\r\n"
                       "Connection: keep-alive\r\n"
                       "Upgrade-Insecure-Requests: 1\r\n"
                       "Cache-Control: max-age=0\r\n\r\n";

    send(client,send_data,strlen(send_data), 0);
    char buffer[500] = {0};

    int ret = recv(client,buffer,500,0);
    if (ret > 0)
    {
        buffer[ret] = 0;
        printf("%s",buffer);
    }
    printf("ret %d", ret);

    closesocket(client);
    WSACleanup();
    return 0;
}