//
// Created by xx on 2018/7/15.
//

#include <cstdio>
#include <winsock.h>
#include "sha1.h"

void print_hex(unsigned char* n, u_32 len)
{
    for (u_32 i = 0; i < len; ++i)
    {
        printf("%02x", *(n+i));
    }
    printf("\n");
}

u_32 left_shift(u_32 n, u_32 bits)
{
    return (((n<<bits)|(n>>(32-bits))) &0xffffffff );
}

void process_one_chunk(unsigned char*chunk,u_32 *h0, u_32 *h1, u_32 *h2, u_32*h3, u_32 *h4)
{
    u_32 *t = (u_32*)chunk;
    u_32 i = 0;
    u_32 w[80];

    for (; i < 16; ++i)
    {
        w[i] = htonl(*t);
        t++;
    }
    for (; i < 80; ++i)
    {
        u_32 tmp = (w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16]);
        w[i] = left_shift(tmp, 1);
    }
    //Initialize hash value for this chunk:
    u_32 a = *h0, b = *h1, c = *h2 ,d = *h3, e = *h4, k,f;
    for (i = 0; i < 80; ++i)
    {
        if (i >= 0 && i <= 19 )
        {
            f = ( (b & c) | ((~ b) & d) );
            k = (0x5A827999UL);
        }
        else if (i >= 20 && i <= 39)
        {
            f = (b ^ c ^ d);
            k = (0x6ED9EBA1UL);
        }
        else if (i >= 40 && i <= 59)
        {
            f = ( (b & c) | (b & d) | (c & d) );
            k = (0x8F1BBCDCUL);
        }
        else if (i >= 60 && i <= 79)
        {
            f = ( b ^ c ^ d );
            k = (0xCA62C1D6UL);
        }

        u_32 temp = (left_shift(a, 5) + f + e + k + w[i]);
        e = d;
        d = c;
        c = (left_shift(b, 30));
        b = a;
        a = temp;
    }
    // Add this chunk's hash to result so far:
    *h0 = *h0 + a;
    *h1 = *h1 + b;
    *h2 = *h2 + c;
    *h3 = *h3 + d;
    *h4 = *h4 + e;
}

u_32 h0 = (0x67452301UL);
u_32 h1 = (0xEFCDAB89UL);
u_32 h2 = (0x98BADCFEUL);
u_32 h3 = (0x10325476UL);
u_32 h4 = (0xC3D2E1F0UL);

void sha_1(u_char *s,u_64 total_len,u_32 *hh)
{
    u_64 msg_len = total_len;
    u_64 msg_len_bits = (msg_len * 8);
    u_32 *tm_bit_len = (u_32*)&msg_len_bits;
    u_32 big_ediant_len [2] = {0};
    big_ediant_len[0] = htonl(tm_bit_len[1]);
    big_ediant_len[1] = htonl(tm_bit_len[0]);

    u_64 rest_len = msg_len % 64;
    u_64 chunk_num = msg_len / 64;
    unsigned char last_chunk[128] = {0};
    memcpy((void*)last_chunk,(void*)&s[chunk_num*64],rest_len);
    last_chunk[rest_len] = 0x80;
    u_32 last_idx = 0;
    if (rest_len + 1 <=56)
    {
        memcpy((void*)&last_chunk[56],&big_ediant_len, 8);
    } else
    {
        memcpy((void*)&last_chunk[120],&big_ediant_len, 8);
        last_idx = 1;
    }
    // print_hex((unsigned char*)last_chunk,64*(last_idx+1));
    u_32 j = 0;
    for (;j < chunk_num; ++j)
    {
        process_one_chunk(&s[64*j],&h0,&h1,&h2,&h3,&h4);
    }
    for (j = 0 ;j <= last_idx; ++j)
    {
        process_one_chunk(&last_chunk[64*j],&h0,&h1,&h2,&h3,&h4);
    }

    hh[0] = htonl(h0);
    hh[1] = htonl(h1);
    hh[2] = htonl(h2);
    hh[3] = htonl(h3);
    hh[4] = htonl(h4);
}

int main()
{
    char s[] = "";// sha1 = da39a3ee5e6b4b0d3255bfef95601890afd80709
    u_64 msg_len = strlen(s);
    u_32 hh[5] = {0};
    sha_1((u_char*)s, msg_len,hh);
    print_hex((unsigned char*)hh, 20);

    return 0;
}