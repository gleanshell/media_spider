#ifndef LIST_H_INCLUDED
#define LIST_H_INCLUDED





#define offset_of(type, member) ( (long) &((type*)0)->member )

#define container_of(type,member,m_ptr) ( (type*)((char*)m_ptr - offset_of(type,member)) )

#define list_for_each(head, tmp) for (tmp = head->next; tmp != head; tmp = tmp->next)
typedef struct list_head
{
    struct list_head* prev;
    struct list_head* next;
}list_head_t;

static inline void list_init(list_head_t*head)
{
    head->next = head;
    head->prev = head;
}

static inline void _list_add(list_head_t* prev, list_head_t *next, list_head_t *cur)
{
    prev->next = cur;
    cur->next = next;

    next->prev = cur;
    cur->prev = prev;
}

static inline void list_add_tail(list_head_t *head, list_head_t *cur)
{
    _list_add(head->prev, head, cur);
}

static inline void list_add_first(list_head_t *head, list_head_t *cur)
{
    _list_add(head, head->next, cur);
}

static inline void list_del(list_head_t *del)
{
    del->prev->next = del->next;
    del->next->prev = del->prev;
}

static inline void list_replace(list_head_t *new_node, list_head_t* old_node)
{
    old_node->prev->next = new_node;
    new_node->next = old_node->next;
    old_node->next->prev = new_node;
    new_node->prev = old_node->prev;
    list_init(old_node);
}


typedef struct test
{
    int data;
    list_head_t node;
}test_t;

#endif // LIST_H_INCLUDED
