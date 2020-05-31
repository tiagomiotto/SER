

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
#include "net/rime/rime.h"
#include "node-id.h"
#include "leds.h"

#define DELAY_MAX (random_rand() % (15 * CLOCK_SECOND) + (5 * CLOCK_SECOND)) *2
#define STATE_OFF 0
#define STATE_ON 1
#define STATE_ACTIVE 2
#define SYNC_NODE_ID 1

struct Message
{
  char data[50];
  int srcID;
  int destID;
  int mode;
  int distance;
};

// Function used to send the Message struct using UDP messages
bool sendMessage(struct simple_udp_connection connection,
                  struct Message *messageTX);

// Function used to populate the structure
void prepareMessage(struct Message* message,char* data, int srcID,int destID, int mode, int distance);

// Function to set the IPv6 address of the node
uip_ipaddr_t* set_global_address(void);

//Function to create the RPL Dag routing
void create_rpl_dag(uip_ipaddr_t *ipaddr);

/* Function called by all the nodes to obtain a IP adress and a ID, and register
 that ID as a service in the servreg_hack_register. In the case of the sync node,
 it also creates the RPL Dag, in order to be the router for the messages in the network
 creating a star Architecture*/
int registerConnection(int ID);

/* Simple function to send a change in state to the sync, in the final version it
ended up unused.*/
void sendStateToSync(struct simple_udp_connection connection, int mydID, int state);
