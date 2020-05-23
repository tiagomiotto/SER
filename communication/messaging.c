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
        printf(" %s, %d, %d, %d\n", messageTX->msg, messageTX->destID,messageTX->srcID, messageTX->code);
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

// static uip_ipaddr_t *set_global_address(void)
// {
//   static uip_ipaddr_t ipaddr;
//   int i;
//   uint8_t state;

//   uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
//   uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
//   uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

//   printf("IPv6 addresses: ");
//   for (i = 0; i < UIP_DS6_ADDR_NB; i++)
//   {
//     state = uip_ds6_if.addr_list[i].state;
//     if (uip_ds6_if.addr_list[i].isused &&
//         (state == ADDR_TENTATIVE || state == ADDR_PREFERRED))
//     {
//       uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
//       printf("\n");
//     }
//   }

//   return &ipaddr;
// }