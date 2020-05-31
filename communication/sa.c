#ifndef _DEFINITIONS_H
#define _DEFINITIONS_H
#include "contiki.h"
#include "net/rime/rime.h"
#include "net/netstack.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/ip/uip.h"
#include "net/rpl/rpl.h"

#include "net/netstack.h"
#include "dev/button-sensor.h"
#include <ctype.h>
#include "dev/leds.h"
#include "cc2420.h"
#include "cc2420_const.h"
#include "dev/spi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "messaging.h"
#endif

#include "dev/serial-line.h"

#define MAX_NODES 255
#define UDP_PORT 1234
#define UNICAST_PORT 1235
#define ID 0

int STATUS = 0; // 0 if not active 1 if active
int distance = -1;
int myID;
struct Message *my_received_message;//Not active
struct Message *my_send_message;
static struct simple_udp_connection unicast_connection;

struct message_list {//active
  struct node *next;
	struct Message message;
};

MEMB(message_memb, struct message_list, MAX_NODES);
LIST(message_list);

void prepareMessage(struct Message message, char* msg, uint8_t srcID,
	uint8_t destID, uint8_t mode, uint8_t distance) {

    message.srcID = srcID;
    message.destID = destID;
    message.mode = mode;
		message.distance = distance;
}

/******************************************************************************/
PROCESS(my_distance, "Measure my distance to the target");
PROCESS(send_message_handler,"Send message to node(s)");
PROCESS(receive_message_handler,"Handle received message");
PROCESS(receive_message, "Receive message from node(s)");

AUTOSTART_PROCESSES(&my_distance, &send_message_handler,
									&receive_message_handler, &receive_message);
/******************************************************************************/
static void receiver(struct simple_udp_connection *c,
				 						 const uip_ipaddr_t *sender_addr,
         	 					 uint16_t sender_port,
         	 					 const uip_ipaddr_t *receiver_addr,
         	 					 uint16_t receiver_port,
         	 					 const uint8_t *data,
         	 					 uint16_t datalen) {

  printf("Data received on port %d from port %d with length %d : %s\n",
         receiver_port, sender_port, datalen, (char *)data);
	process_post(&receive_message,
               PROCESS_EVENT_CONTINUE, &data);
}
/******************************************************************************/
int generate_random_distance(int pos) {
  time_t t;
	srand((unsigned) time(&t));
	int new_pos;

	int pos_array[20] = {0,0,0,0,0,0,0,0,0,
							 		 1,1,1,1,1,
							 	 	 2,2,2,
							 	 	 3,3,
							 	 	 4};

	if(rand() % 2){
		new_pos = pos - pos_array[rand() % 20];
	}else{
		new_pos = pos + pos_array[rand() % 20];
	}
	if(new_pos < 0)
	new_pos = abs(new_pos);

	return new_pos;
}
/*--------------------------------------------------------------------DISTANCE*/
PROCESS_THREAD(my_distance, ev, data) {
	static struct etimer et;
	time_t t;

	//first distance
	if(distance == -1)
	{
		srand((unsigned) time(&t));
		distance = rand() % 20;
	}

	PROCESS_BEGIN();

	while (1) {
		//generate a new distance every ten seconds
		etimer_set(&et, CLOCK_SECOND * 10);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		distance = generate_random_distance(distance);
	}
  PROCESS_END();
}
/*----------------------------------------------------------------SEND_MESSAGE*/
static void tcpip_handler(void) {
  char *appdata;

  if(uip_newdata()) {
    appdata = (char *)uip_appdata;
    appdata[uip_datalen()] = 0;
    PRINTF("DATA recv '%s' from ", appdata);
    PRINTF("%d",
           UIP_IP_BUF->srcipaddr.u8[sizeof(UIP_IP_BUF->srcipaddr.u8) - 1]);
    PRINTF("\n");
#if SERVER_REPLY
    PRINTF("DATA sending reply\n");
    uip_ipaddr_copy(&server_conn->ripaddr, &UIP_IP_BUF->srcipaddr);
    uip_udp_packet_send(server_conn, "Reply", sizeof("Reply"));
    uip_create_unspecified(&server_conn->ripaddr);
#endif
  }
}

/******************************************************************************/
static void print_local_addresses(void) {
  int i;
  uint8_t state;

  PRINTF("IPv6 addresses: ");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(state == ADDR_TENTATIVE || state == ADDR_PREFERRED) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
      /* hack to make address "final" */
      if (state == ADDR_TENTATIVE) {
	uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
      }
    }
  }
}
/******************************************************************************/
PROCESS_THREAD(send_message_handler, ev, data) {
	static struct etimer et;
	uip_ipaddr_t addr;

	PROCESS_BEGIN();

	servreg_hack_init();
	simple_udp_register(&unicast_connection, UDP_PORT,
											NULL, UDP_PORT, receiver);

	myID = registerConnection(ID);

	SENSORS_ACTIVATE(button_sensor);
	while(1) {
		//active node
		if (STATUS == 1) {
		/*

	#if 0

	   uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 1);
	#elif 1

	  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0x00ff, 0xfe00, 1);
	#else

	  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
	  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
	#endif

	  uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);
	  root_if = uip_ds6_addr_lookup(&ipaddr);
	  if(root_if != NULL) {
	    rpl_dag_t *dag;
	    dag = rpl_set_root(RPL_DEFAULT_INSTANCE,(uip_ip6addr_t *)&ipaddr);
	    uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
	    rpl_set_prefix(dag, &ipaddr, 64);
	    PRINTF("created a new RPL dag\n");
	  } else {
	    PRINTF("failed to create a new RPL DAG\n");
	  }

	  print_local_addresses();

	  NETSTACK_MAC.off(1);

	  server_conn = udp_new(NULL, UIP_HTONS(UDP_CLIENT_PORT), NULL);
	  if(server_conn == NULL) {
	    PRINTF("No UDP connection available, exiting the process!\n");
	    PROCESS_EXIT();
	  }
	  udp_bind(server_conn, UIP_HTONS(UDP_SERVER_PORT));

	  PRINTF("Created a server connection with remote address ");
	  PRINT6ADDR(&server_conn->ripaddr);
	  PRINTF(" local/remote port %u/%u\n", UIP_HTONS(server_conn->lport),
	         UIP_HTONS(server_conn->rport));
	  while(1) {
	    PROCESS_YIELD();
	    if(ev == tcpip_event) {
	      tcpip_handler();
	    } else if (ev == sensors_event && data == &button_sensor) {
	      PRINTF("Initiaing global repair\n");
	      rpl_repair_root(RPL_DEFAULT_INSTANCE);
	    }
	  }
		*/
			etimer_set(&et, CLOCK_SECOND * 30);
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

			uip_create_linklocal_allnodes_mcast(&addr);
			prepareMessage(*my_send_message, "", myID, 0, 0, distance);
			simple_udp_sendto(&unicast_connection, my_send_message, sizeof(my_send_message), &addr);
		}
	}
  PROCESS_END();
}


/*-------------------------------------------------------------RECEIVE_MESSAGE*/
PROCESS_THREAD(receive_message, ev, data) {
	struct message_list *m, *n, *min_distance_p;


  PROCESS_BEGIN();
	//is active node
	while (1) {
	PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
	if (STATUS == 1) {
		struct Message *inMsg = (struct Message *)data;

		m = memb_alloc(&message_memb);
		m->message = *inMsg;

		if (m->message.mode == 2) {
				STATUS = 0;
				char buffer[10];
				sprintf(buffer, "%d,%d", m->message.srcID, 2);
				prepareMessage(*my_send_message, buffer, myID, 1, 4, 0);
				sendMessage(unicast_connection,my_send_message);

		} else if (m->message.mode == 0)
			list_add(message_list, m);
	}
	//is not active node
	else {
		struct Message *inMsg = (struct Message *)data;
		m->message = *inMsg;

			if (m->message.mode == 1) {
				//funtion to fake actuator
				prepareMessage(*my_send_message, "", myID, m->message.destID, 2, 0);
				sendMessage(unicast_connection, my_send_message);
				STATUS = 1;
			} else if (distance < m->message.distance) {
				prepareMessage(*my_send_message, "", myID, m->message.destID, 0, distance);
				sendMessage(unicast_connection, my_send_message);
			}
		}
	}
  PROCESS_END();
}
/*-----------------------------------------------------RECEIVE_MESSAGE_HANDLER*/
PROCESS_THREAD(receive_message_handler, ev, data) {
	static struct etimer et;
	struct message_list *n,*min_distance_p;
	int min_distance;

	PROCESS_BEGIN();
		while (1) {
			PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_CONTINUE);
			etimer_set(&et, CLOCK_SECOND * 5);
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

			min_distance = 10000;

			for (n = list_head(message_list); n != NULL; n = list_item_next(n)) {
				if (n->message.distance < min_distance && n->message.distance < distance) {
					min_distance = n->message.distance;
					min_distance_p = n;
				}
			}

			if (min_distance_p != NULL) {
				if ((float) min_distance_p->message.distance/distance < 0.75) {
					prepareMessage(*my_send_message, "", myID, min_distance_p->message.destID, 1, 0);
					sendMessage(unicast_connection, my_send_message);
				}
			}
		}
	PROCESS_END();
}
