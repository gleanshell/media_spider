
typedef unsigned int uint32_t;
typedef struct map_entry
{
    void *key;
    void *val;
    struct map_entry *next;
} map_entry;

typedef unsigned int (*hash_Fn)(void*key);
typedef int (*equal_Fn)(void*k1, void*k2);

typedef struct hash_map
{
    hash_Fn hashf;
    equal_Fn equalf;
    map_entry **bucket;
    unsigned int mask;
    int used;
    int size;
}hash_map;


void map_init(hash_map *m, hash_Fn hash_fn, equal_Fn equal_fn, int bucket_size, unsigned int _mask);
int map_put(hash_map *m, map_entry*e);
map_entry* map_get(hash_map *m, void *key);
int map_del(hash_map *m, void*key);
int map_for_each();
