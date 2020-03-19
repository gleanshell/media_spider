#ifndef __EVT_H_
#define __EVT_H_


#include<winsock.h>
#include "list.h"

typedef struct timer_evt
{
    int timer_id;
    long when_sec;
    long when_ms;
    long interval_ms;
    void *data;
    void (*f)(void*);
    list_head_t node;
}timer_evt_t;

typedef struct read_evt
{
    int fd;
    void *data;
    void (*readf)(void*);
}read_evt_t;

typedef struct evt_loop_mgr
{
    int max_fd_num;
    fd_set fd,_fd;
    read_evt_t *rd_evt;
    list_head_t timer_head;
}evt_loop_mgr_t;

void init_evt_mgr(evt_loop_mgr_t*evt_mgr, int max_fd_num );

int create_read_evt(evt_loop_mgr_t *ev_mgr, void(*f)(void*), int fd, void*data);
int create_timer_evt(int timer_id,evt_loop_mgr_t *evt_mgr, void(*f)(void*), long interval_ms, void*data);

#endif // __EVT_H_
