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
//#include "cc2420.h"
//#include "cc2420_const.h"
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

int STATUS = 1; // 0 if not active 1 if active
volatile int distance;
int myID;
bool off=false;
struct Message my_received_message; //Not active
struct Message my_send_message;
static struct simple_udp_connection unicast_connection;

struct message_list
{ //active
	struct node *next;
	struct Message message;
};

MEMB(message_memb, struct message_list, MAX_NODES);
LIST(message_list);

// void prepareMessage(struct Message *message, char* msg, uint8_t srcID,
// 	uint8_t destID, uint8_t mode, uint8_t distance) {

//     message.srcID = srcID;
//     message.destID = destID;
//     message.mode = mode;
// 	message.distance = distance;
// }

/******************************************************************************/
PROCESS(my_distance, "Measure my distance to the target");
PROCESS(send_message_handler, "Send message to node(s)");
PROCESS(receive_message_handler, "Handle received message");
PROCESS(receive_message, "Receive message from node(s)");

AUTOSTART_PROCESSES(&my_distance, &send_message_handler, /*&receive_message_handler,*/ &receive_message);
/******************************************************************************/
static void receiver(struct simple_udp_connection *c,
					 const uip_ipaddr_t *sender_addr,
					 uint16_t sender_port,
					 const uip_ipaddr_t *receiver_addr,
					 uint16_t receiver_port,
					 const uint8_t *data,
					 uint16_t datalen)
{

	printf("Data received from ");
	uip_debug_ipaddr_print(sender_addr);
	struct Message *inMsg = (struct Message *)data;
	my_received_message = *inMsg;
	printf(" on port %d from port %d, with ID %d, with length %d: '%d'\n",
		   receiver_port, sender_port, my_received_message.srcID, datalen, my_received_message.mode);

	process_post(&receive_message,
				 PROCESS_EVENT_CONTINUE, data);
}
/******************************************************************************/
int generate_random_distance(int pos)
{
	time_t t;
	srand((unsigned)time(&t));
	int new_pos;

	int pos_array[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0,
						 1, 1, 1, 1, 1,
						 2, 2, 2,
						 3, 3,
						 4};

	if (rand() % 2)
	{
		new_pos = pos - pos_array[rand() % 20];
	}
	else
	{
		new_pos = pos + pos_array[rand() % 20];
	}
	if (new_pos < 0)
		new_pos = abs(new_pos);
	printf("%d\n", new_pos);
	return new_pos;
}
/*--------------------------------------------------------------------DISTANCE*/
PROCESS_THREAD(my_distance, ev, data)
{

	PROCESS_BEGIN();
	static struct etimer et;
	time_t t;

	srand((unsigned)time(&t));
	distance = rand() % 20;
	while (1)
	{
		//generate a new distance every ten seconds
		etimer_set(&et, CLOCK_SECOND * 10);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		distance = generate_random_distance(distance);
	}
	PROCESS_END();
}
PROCESS_THREAD(send_message_handler, ev, data)
{
	PROCESS_BEGIN();
	static struct etimer et;
	uip_ipaddr_t addr;

	servreg_hack_init();
	simple_udp_register(&unicast_connection, UDP_PORT,
						NULL, UDP_PORT, receiver);

                          printf("Delay max %d\n", DELAY_MAX);
  static struct etimer timer;
  etimer_set(&timer, DELAY_MAX);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

	myID = registerConnection(ID);

	//SENSORS_ACTIVATE(button_sensor);
	etimer_set(&et, CLOCK_SECOND * 30);
	while (1)
	{
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		etimer_reset(&et);
		//active node
		if (STATUS == 1)
		{
			uip_create_linklocal_allnodes_mcast(&addr);
			prepareMessage(&my_send_message, "", myID, 0, 0, distance);
			//printf(" %s, %d, %d, %d\n", my_send_message.data, my_send_message.destID, my_send_message.srcID, my_send_message.mode);
			simple_udp_sendto(&unicast_connection, &my_send_message, sizeof(struct Message) + 1, &addr);
		}
	}
	PROCESS_END();
}

/*-------------------------------------------------------------RECEIVE_MESSAGE*/
PROCESS_THREAD(receive_message, ev, data)
{

	PROCESS_BEGIN();
	struct message_list *m, *n, *min_distance_p;
	//is active node
	while (1)
	{
		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
        struct Message *inMsg = (struct Message *)data;

		if (inMsg->mode == 3 && inMsg->srcID == 1){
			if(strcmp(inMsg->data,"on")==0) off=false;
			if(strcmp(inMsg->data,"off")==0) off=true;
			prepareMessage(&my_send_message, inMsg->data, myID, inMsg->srcID, 3, 0);
			sendMessage(unicast_connection, &my_send_message);
		}

		if(off) {
			printf("I was turned off\n");
			continue; 
		}

		if (STATUS == 1)
		{
			
			
			
			m = memb_alloc(&message_memb);
			m->message = *inMsg;
		
			if (m->message.mode == 2)
			{
				STATUS = 0;
				char buffer[10];
				sprintf(buffer, "%d,%d", m->message.srcID, 2);
				prepareMessage(&my_send_message, buffer, m->message.srcID, 1, 4, 0);
				sendMessage(unicast_connection, &my_send_message);
			}
			else if (m->message.mode == 0){
                list_add(message_list, m);
                process_post(&receive_message_handler,
                PROCESS_EVENT_CONTINUE, data);
                // prepareMessage(&my_send_message, "", myID, m->message.srcID, 1, 0);
                // sendMessage(unicast_connection, &my_send_message);
				}
		}
		//is not active node
		else
		{
			
            printf("Mode received: %d\n", inMsg->mode);
			if (inMsg->mode == 1)
			{
				//funtion to fake actuator
				prepareMessage(&my_send_message, "", myID, inMsg->srcID, 2, 0);
				sendMessage(unicast_connection, &my_send_message);
				STATUS = 1;
			}
			else if (distance < inMsg->distance)
			{
				prepareMessage(&my_send_message, "", myID, inMsg->srcID, 0, distance);
				sendMessage(unicast_connection, &my_send_message);
			}
		}
	}
	PROCESS_END();
}
/*-----------------------------------------------------RECEIVE_MESSAGE_HANDLER*/
PROCESS_THREAD(receive_message_handler, ev, data)
{

	PROCESS_BEGIN();
	static struct etimer et;
	struct message_list *n, *min_distance_p;
	int min_distance;

	while (1)
	{
		PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_CONTINUE);
		etimer_set(&et, CLOCK_SECOND * 5);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

		min_distance = 10000;

		for (n = list_head(message_list); n != NULL; n = list_item_next(n))
		{
			if (n->message.distance < min_distance && n->message.distance < distance)
			{
				min_distance = n->message.distance;
				min_distance_p = n;
			}
		}

		if (min_distance_p != NULL)
		{
			if ((float)min_distance_p->message.distance / distance < 0.75)
			{
				prepareMessage(&my_send_message, "", myID, min_distance_p->message.destID, 1, 0);
				sendMessage(unicast_connection, &my_send_message);
			}
		}
	}
	PROCESS_END();
}
