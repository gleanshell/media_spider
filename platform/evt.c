#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "evt.h"

void get_time(long *sec,long *millisec)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    *sec = tv.tv_sec;
    *millisec = tv.tv_usec/1000;
}

void init_evt_mgr(evt_loop_mgr_t*evt_mgr, int max_fd_num)
{
    evt_mgr->rd_evt = (read_evt_t*)calloc(max_fd_num, sizeof(read_evt_t));
    if (NULL == evt_mgr->rd_evt)
    {
        printf("calloc err\n");
        return -1;
    }

    int i = 0;
    for(;i < max_fd_num; ++i)
    {
        evt_mgr->rd_evt[i].fd = -1;
    }
    evt_mgr->max_fd_num = max_fd_num;
    list_init(&evt_mgr->timer_head);
    FD_ZERO(&evt_mgr->fd);
    printf("ev loop mgr init success!\n");
    return 0;
}
int create_read_evt(evt_loop_mgr_t *ev_mgr, void(*f)(void*), int fd, void*data)
{
    if(fd >= ev_mgr->max_fd_num)
    {
        printf("fd err\n");
        return -1;
    }

    ev_mgr->rd_evt[fd].data = data;
    ev_mgr->rd_evt[fd].readf = f;
    ev_mgr->rd_evt[fd].fd = fd;
    FD_SET(fd, &ev_mgr->fd);

    return 0;
}
int create_timer_evt(int timer_id,evt_loop_mgr_t *evt_mgr, void(*f)(void*), long interval_ms, void*data)
{
    timer_evt_t *te = (timer_evt_t *)calloc(1, sizeof(timer_evt_t));
    if (NULL == te)
    {
        printf("calloc err\n");
        return -1;
    }
    te->f = f;
    te->data = data;
    list_init(&te->node);
    te->interval_ms = interval_ms;
    te->timer_id = timer_id;
    long now_sec,now_ms;
    get_time(&now_sec, &now_ms);
    te->when_sec = (now_sec + interval_ms /1000);
    te->when_ms = (now_ms + interval_ms%1000);
    if (te->when_ms >= 1000)
    {
        te->when_sec += 1;
        te->when_ms -= 1000;
    }

    list_add_first(&evt_mgr->timer_head, &te->node);

    return 0;
}

int get_near_timer(evt_loop_mgr_t*e, long *sec, long *millisec)
{
    timer_evt_t *t1 = container_of(timer_evt_t, node, e->timer_head.next);
    long near_sec = t1->when_sec;
    long near_ms = t1->when_ms;

    list_head_t *head = &e->timer_head, *tmp;
    int n = 0;
    list_for_each(head,tmp)
    {
        timer_evt_t *te = container_of(timer_evt_t, node, tmp);
        if (te->when_sec < near_sec || te->when_sec == near_sec && te->when_ms < near_ms)
        {
            near_sec = te->when_sec;
            near_ms = te->when_ms;
        }
        ++n;
    }
    *sec = near_sec;
    *millisec = near_ms;
    return n;
}

void process_timer_evt(evt_loop_mgr_t*e)
{
    list_head_t *head = &e->timer_head, *tmp;
    long now_sec,now_ms;
    get_time(&now_sec, &now_ms);
    list_for_each(head, tmp)
    {
        timer_evt_t *te = container_of(timer_evt_t, node, tmp);
        if (te->when_sec < now_sec || te->when_sec == now_sec && te->when_ms <= now_ms)
        {
            te->f(te->data);
            te->when_sec += (te->interval_ms/1000);
            te->when_ms += (te->interval_ms%1000);
            if (te->when_ms >= 1000)
            {
                te->when_ms -= 1000;
                te->when_sec += 1;
            }
        }
    }
}

void evt_loop(evt_loop_mgr_t*e)
{
    //file evt
    long sec,millisec, near_sec,near_ms;

    struct timeval tv;
    struct timeval *timeout = &tv;

    int ret;
    while(1)
    {
        get_time(&sec, &millisec);
        ret = get_near_timer(e, &near_sec, &near_ms);
        if (ret == 0)
        {
            timeout = NULL;
        }
        else
        {
            if (near_sec >= sec)
            {
                timeout->tv_sec = near_sec - sec;
                if ( near_ms > millisec)
                {
                    timeout->tv_usec = (near_ms- millisec) *1000;
                }
                else
                {
                    timeout->tv_usec = (near_ms +1000- millisec)*1000;
                    timeout->tv_sec -= 1;
                }
            }
            else
            {
                timeout->tv_sec = 0;
                timeout->tv_usec = 0;
            }


            if (timeout->tv_sec < 0)
            {
                timeout->tv_sec = 0;
            }
            if (timeout->tv_usec < 0)
            {
                timeout->tv_usec = 0;
            }
        }
        e->_fd = e->fd;
        ret = select(e->max_fd_num, &e->_fd, NULL, NULL, timeout);
        if (ret > 0)
        {
            int i =0;
            for(;i < e->max_fd_num; ++i)
            {
                if (e->rd_evt[i].fd != -1 && FD_ISSET(e->rd_evt[i].fd, &e->_fd))
                {
                    e->rd_evt[i].readf(e->rd_evt[i].data);
                }
            }
        }

        //timer evt
        process_timer_evt(e);

    }

    //timer evt
}
