#include "messaging.h"

void sendMessage(struct simple_udp_connection connection,
                  struct Message *messageTX)
{
    uip_ipaddr_t *addr;
    addr = servreg_hack_lookup(messageTX->destID);
    if (addr != NULL)
    {
    
        printf("Sending unicast to ");
        uip_debug_ipaddr_print(addr);
        printf(" %s, %d, %d, $d\n", messageTX->msg, messageTX->destID,messageTX->srcID, messageTX->code);
        simple_udp_sendto(&connection, messageTX, sizeof(struct Message) + 1, addr);
    }
    else
    {
        printf("Service %d not found\n", messageTX->destID);
    }
}

struct Message prepareMessage(char* data, uint8_t srcID,uint8_t destID, uint8_t code){
    struct Message messageTx;
    strcpy(messageTx.msg, data);
    messageTx.srcID=srcID;
    messageTx.destID=destID;
    messageTx.code=code;
    return messageTx;
}

