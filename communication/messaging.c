#include "messaging.h"

void sendMessage(struct simple_udp_connection connection,
                  struct Message *messageTX)
{
    uip_ipaddr_t *addr;
    addr = servreg_hack_lookup(messageTX->id);
    if (addr != NULL)
    {
    
        printf("Sending unicast to ");
        uip_debug_ipaddr_print(addr);
        printf("\n");
        simple_udp_sendto(&connection, &messageTX, sizeof(struct Message) + 1, addr);
    }
    else
    {
        printf("Service %d not found\n", messageTX->id);
    }
}

struct Message prepareMessage(char* data, uint8_t txID, uint8_t code){
    struct Message messageTx;
    strcpy(messageTx.msg, data);
    messageTx.id=txID;
    return messageTx;
}

