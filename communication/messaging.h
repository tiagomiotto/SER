#include "servreg-hack.h"
#include "simple-udp.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"
#include "net/rpl/rpl.h"

struct Message
{
  char msg[50];
  uint8_t srcID;
  uint8_t destID;
  uint8_t code;
};

void sendMessage(struct simple_udp_connection connection,
                  struct Message *messageTX);

struct Message prepareMessage(char* data, uint8_t srcID,uint8_t destID, uint8_t code);

//static uip_ipaddr_t *set_global_address(void);

static void create_rpl_dag(uip_ipaddr_t *ipaddr);