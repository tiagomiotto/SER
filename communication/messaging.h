

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


#define DELAY_MAX (random_rand() % (20 * CLOCK_SECOND) + (5 * CLOCK_SECOND))
#define STATE_OFF 0
#define STATE_ON 1
#define STATE_ACTIVE 2

struct Message
{
  char msg[50];
  int srcID;
  int destID;
  int code;
};

void sendMessage(struct simple_udp_connection connection,
                  struct Message *messageTX);

struct Message prepareMessage(char* data, int srcID,int destID, int code);

uip_ipaddr_t* set_global_address(void);

void create_rpl_dag(uip_ipaddr_t *ipaddr);

int registerConnection(int ID);

void sendStateToSync(struct simple_udp_connection connection, int mydID, int state);
