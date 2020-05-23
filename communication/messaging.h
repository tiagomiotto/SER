

#include "servreg-hack.h"
#include "simple-udp.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"
#include "net/rpl/rpl.h"
#include "stdbool.h"
#include <time.h>
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"


#define DELAY_MAX (random_rand() %(5 * CLOCK_SECOND))
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

uip_ipaddr_t* set_global_address(void);

void create_rpl_dag(uip_ipaddr_t *ipaddr);

uip_ipaddr_t *registerConnection(uint8_t ID);
