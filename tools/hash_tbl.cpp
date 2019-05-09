#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hash_tbl.h"

/* MurmurHash2, by Austin Appleby
 * Note - This code makes a few assumptions about how your machine behaves -
 * 1. We can read a 4-byte value from any address without crashing
 * 2. sizeof(int) == 4
 *
 * And it has a few limitations -
 *
 * 1. It will not work incrementally.
 * 2. It will not produce the same results on little-endian and big-endian
 *    machines.
 */
unsigned int murmur_hash(const void *key, int len) {
    /* 'm' and 'r' are mixing constants generated offline.
     They're not really 'magic', they just happen to work well.  */
    uint32_t seed = 5381;
    const uint32_t m = 0x5bd1e995;
    const int r = 24;

    /* Initialize the hash to a 'random' value */
    uint32_t h = seed ^ len;

    /* Mix 4 bytes at a time into the hash */
    const unsigned char *data = (const unsigned char *)key;

    while(len >= 4) {
        uint32_t k = *(uint32_t*)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    /* Handle the last few bytes of the input array  */
    switch(len) {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0]; h *= m;
    };

    /* Do a few final mixes of the hash to ensure the last few
     * bytes are well-incorporated. */
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return (unsigned int)h;
}

unsigned int str_hash(void*key)
{
    return murmur_hash(key, 20);
}

/* Thomas Wang's 32 bit Mix Function */
unsigned int dictIntHashFunction(unsigned int key)
{
    key += ~(key << 15);
    key ^=  (key >> 10);
    key +=  (key << 3);
    key ^=  (key >> 6);
    key += ~(key << 11);
    key ^=  (key >> 16);
    return key;
}

unsigned int int_hash(void*key)
{
    unsigned int k = *(unsigned int*)key;
    return dictIntHashFunction(k);
}
int int_equal_f(void*k1, void*k2)
{
    unsigned int key1 = *(unsigned int*)k1;
    unsigned int key2 = *(unsigned int*)k2;
    return (key1 == key2 ? 1: 0);
}
int str_equal_f(void*k1, void*k2)
{
    return (0 == memcmp(k1, k2, 20));
}
void map_init(hash_tbl *m, hash_Fn hash_fn, equal_Fn equal_fn, unsigned int bucket_size, unsigned int _mask)
{
    memset(m, 0, sizeof(hash_tbl) );
    m->bucket = (map_entry **) malloc(bucket_size * sizeof(map_entry*) );
    memset(m->bucket, 0, bucket_size * sizeof(map_entry*) );
    m->equalf = equal_fn;
    m->hashf = hash_fn;
    m->mask = _mask;
    m->cur = -1;
    m->size = bucket_size;
}

int map_put(hash_tbl *m, map_entry *entry)
{
    unsigned int hash = m->hashf(entry->key);
    int pos = hash & m->mask;
    map_entry *en = (map_entry*)malloc(sizeof(map_entry));
    en->key = entry->key;
    en->val = entry->val;

    map_entry *e = m->bucket[pos];
    while(e)
    {
        if (m->equalf(e->key, entry->key))
        {
            return 1;//exist
        }
        e = e->next;
    }
    en->next = m->bucket[pos];
    m->bucket[pos] = en;
    m->used += 1;
    return 0;
}

map_entry * map_get(hash_tbl *m, void*key)
{
    unsigned int hash = m->hashf(key);
    int pos = hash & m->mask;
    map_entry *e = m->bucket[pos];
    int c = 0;
    while(e)
    {
        if (m->equalf(e->key, key))
        {
            //printf("=====  c = (%d) ===", c);
            return e;
        }
        e = e->next;
        ++ c;
    }
    return NULL;
}

map_entry * map_del(hash_tbl *m, void*key)
{
    unsigned int hash = m->hashf(key);
    int pos = hash & m->mask;
    map_entry *e = m->bucket[pos];
    map_entry **prev = &m->bucket[pos];
    while(e)
    {
        if (m->equalf(e->key, key))
        {
            *prev = e->next;
            m->used -= 1;
            return e;
        }
        *prev = e;
        e = e->next;
    }
    return NULL;
}


int maintest()
{
    hash_tbl _map, *m = &_map;
    map_init(m, str_hash, str_equal_f, 1<<16, (1<<16) -1 );
    size_t str_len = 20;

    for (int i=0; i < 50; ++i)
    {
        map_entry tmp;
        tmp.key = malloc(str_len);
        memset(tmp.key, 0, str_len);
        sprintf((char*)tmp.key, "abcdefg-%d", i);
        tmp.val = malloc(sizeof(unsigned int));
        int *v = (int*)tmp.val;
        *v = 123 + i;
        map_put(m, &tmp);
    }
    for (int j=0; j < 50; ++j)
    {
        void *key = malloc(str_len);
        memset(key, 0, str_len);
        sprintf((char*)key, "abcdefg-%d", j);

        map_entry *f = map_get(m,key);
        if (NULL != f)
        {
            printf("get a key: %s -> val: %d\n", key, *(int*)(f->val));
        }
    }

    printf("\n================================================\n");
    map_entry *e =  NULL;
    map_for_each(m, e)
    {
        printf("map_for_each key: %s -> val:%d\n", (char*)e->key, *(int*)e->val );
    }
    printf("\n========================delete one ========================\n");

    for (int j=0; j < 49; ++j)
    {
        void *key = malloc(str_len);
        memset(key, 0, str_len);
        sprintf((char*)key, "abcdefg-%d", j);

        map_entry *f = map_del(m,key);
        if (NULL != f)
        {
            printf("del a key: %s -> val: %d\n", key, *(int*)(f->val));
            free(f->val);
            f->val = NULL;

            free(f->key);
            f->key = NULL;

            free(f);
            f = NULL;
        }
    }

    printf("-------------erro?-----------\n");
    map_entry *e1 =  NULL;
    map_for_each(m, e1)
    {
        printf("map_for_each key: %s -> val:%d\n", (char*)e1->key, *(int*)e1->val );
    }
}
