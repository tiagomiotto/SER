#include "messaging.h"

void sendMessage(simple_udp_connection connection,
                  struct Message *messageTX)
{
    uip_ipaddr_t *addr;
    addr = servreg_hack_lookup(my_messageRX->id);
    if (addr != NULL)
    {
        my_message.id = ID;
        printf("Sending unicast to ");
        uip_debug_ipaddr_print(addr);
        printf("\n");
        simple_udp_sendto(&unicast_connection, &my_message, sizeof(struct Message) + 1, addr);
    }
    else
    {
        printf("Service %d not found\n", my_messageRX->id);
    }
}

struct Message prepareMessage(char* data, uint8_t txID, uint8_t code){
    struct Message messageTx= {*data,txID};
    return messageTx;
}

