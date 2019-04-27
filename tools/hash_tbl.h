
typedef unsigned int uint32_t;
typedef struct map_entry
{
    void *key;
    void *val;
    struct map_entry *next;
} map_entry;

typedef unsigned int (*hash_Fn)(void*key);
typedef int (*equal_Fn)(void*k1, void*k2);

typedef struct hash_tbl
{
    hash_Fn hashf;
    equal_Fn equalf;
    map_entry **bucket;
    unsigned int mask;
    int cur;
    int used;
    int size;
}hash_tbl;


void map_init(hash_tbl *m, hash_Fn hash_fn, equal_Fn equal_fn, int bucket_size, unsigned int _mask);
int map_put(hash_tbl *m, map_entry*e);
map_entry* map_get(hash_tbl *m, void *key);
int map_del(hash_tbl *m, void*key);

static map_entry *get_next_collision(hash_tbl *m)
{
    for(int j = (m->cur+1); j < m->size; ++j)
    {
        if (NULL != m->bucket[j])
        {
            m->cur = j;
            return m->bucket[j];
        }
    }
    return NULL;
}

static void map_for_each1(hash_tbl *m, map_entry *e)
{
    for (m->cur = -1,e = get_next_collision(m);(e != NULL) && (m->cur < m->size); (e = e->next),(e == NULL ? (e = get_next_collision(m)):e=e ) )
    {

    }
}

#define map_for_each(map, entry) for (m->cur = -1,e = get_next_collision(m);(e != NULL) && (m->cur < m->size); (e = e->next),(e == NULL ? (e = get_next_collision(m)):e=e ))

unsigned int int_hash(void*key);
int int_equal_f(void*k1, void*k2);
