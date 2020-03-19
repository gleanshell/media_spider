#include <stdio.h>
#include <stdlib.h>
#include "evt.h"
#include "dht_spider.h"

void timerf(void*data)
{
    printf("test timer runing...\n");
}

int main()
{
    evt_loop_mgr_t ev,*e = &ev;
    init_evt_mgr(e, 8000);

    dht_print("ok dht spider\n");
    node_t node = {0};
    int ret = init_self_node(&node);
    if (-1 == ret)
    {
        clear_node(&node);
        return -1;
    }
    init_route_table(&node);

    //create socket evt
    printf("rcv socket fd (%d)\n", node.recv_socket);
    create_read_evt(e, rcv_msg, node.recv_socket,&node);


    //ping timer
    create_timer_evt(0, e, period_ping, 3000, &node);
    //find node timer
    create_timer_evt(1, e, find_neighbor, 5000, &node);
    //query msg timeout timer
    create_timer_evt(2, e, msg_timeout_handler, 5000, &node);

    //save file timer
    create_timer_evt(3, e,save_route_tbl_to_file ,10000,&node);

    evt_loop(e);

    return 0;
}
