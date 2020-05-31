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
#define ID 100

int STATUS = 0; // 0 if not active 1 if active
volatile int distance;
int myID;
bool off = false;
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
void deleteList();
void actuatorHandler();
/******************************************************************************/
PROCESS(my_distance, "Measure my distance to the target");
PROCESS(send_message_handler, "Send message to node(s)");
PROCESS(receive_message, "Receive message from node(s)");
PROCESS(receive_message_handler, "Handle received message");

AUTOSTART_PROCESSES(&my_distance, &send_message_handler, &receive_message, &receive_message_handler);
/******************************************************************************/
static void receiver(struct simple_udp_connection *c,
					 const uip_ipaddr_t *sender_addr,
					 uint16_t sender_port,
					 const uip_ipaddr_t *receiver_addr,
					 uint16_t receiver_port,
					 const uint8_t *data,
					 uint16_t datalen)
{
	process_post(&receive_message,
				 PROCESS_EVENT_CONTINUE, data);
}
/******************************************************************************/
/* A function to simulate the data given from the distance sensor, since we are working
 inside a simulator*/
int generate_random_distance(int pos)
{
	time_t t;
	srand(myID * random_rand() % 15);
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
	printf("Target distance: %d\n", new_pos);
	return new_pos;
}
/*----------------DISTANCE------------------*/
/* A Proccess responsible to periodically read the distance from the sensor*/
PROCESS_THREAD(my_distance, ev, data)
{

	PROCESS_BEGIN();
	static struct etimer et;
	time_t t;

	srand(myID * random_rand() % 15);
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

/* A Proccess responsible to periodically send the broadcast message if it is the
  active node*/
PROCESS_THREAD(send_message_handler, ev, data)
{
	PROCESS_BEGIN();
	static struct etimer et;
	uip_ipaddr_t addr;
	random_init(node_id);
	servreg_hack_init();
	simple_udp_register(&unicast_connection, UDP_PORT,
						NULL, UDP_PORT, receiver);


	myID = registerConnection(ID);
	printf("Delay a few seconds before starting, so the routing table can update %d\n", DELAY_MAX);
  	static struct etimer timer;
  	etimer_set(&timer, DELAY_MAX);
  	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
	if (myID == 101 || myID == 102) //Caso o sync seja iniciado primeiro
	{
		printf("I'm the starting active\n");
		STATUS = 1;

		prepareMessage(&my_send_message, "", myID, 1, 4, 0);
		while(!sendMessage(unicast_connection, &my_send_message)){
			printf("Waiting a little more and trying again\n");
			etimer_set(&timer, DELAY_MAX);
  			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
		}
	}

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
			
			simple_udp_sendto(&unicast_connection, &my_send_message, sizeof(struct Message) + 1, &addr);
		}
	}
	PROCESS_END();
}

/*--------RECEIVE_MESSAGE--------------------*/
/* Proccess to handle the multiple types of message received*/
PROCESS_THREAD(receive_message, ev, data)
{

	PROCESS_BEGIN();
	struct message_list *m, *n, *min_distance_p;
	//is active node
	while (1)
	{
		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
		struct Message *inMsg = (struct Message *)data;

		// Handle the ON/OFF message from the sync node
		if (inMsg->mode == 3 && inMsg->srcID == SYNC_NODE_ID)
		{
			if (strcmp(inMsg->data, "on") == 0)
				off = false;
			if (strcmp(inMsg->data, "off") == 0)
				off = true;
			prepareMessage(&my_send_message, inMsg->data, myID, inMsg->srcID, 3, 0);
			sendMessage(unicast_connection, &my_send_message);
		}

		// If turned off by the sync node, I can't become active, so the messages
		// from the active node aren't supposed to be read
		if (off)
		{
			printf("I was turned off\n");
			continue;
		}
		//is the active node
		if (STATUS == 1)
		{

			m = memb_alloc(&message_memb);
			m->message = *inMsg;

			if (m->message.mode == 2)
			{
				// The new active node is setup, turn off my router and let the sync node know
				// who is the new active node	
				actuatorHandler(0);
				STATUS = 0;
				char buffer[10];
				sprintf(buffer, "%d,%d", m->message.srcID, 2);
				prepareMessage(&my_send_message, buffer, m->message.srcID, SYNC_NODE_ID, 4, 0);
				sendMessage(unicast_connection, &my_send_message);
				deleteList();
			}
			else if (m->message.mode == 0)
			{
				// A node responded to my broadcast that it has a smaller distance to target than me
				// add it to the list and let the other proccess know that there is at least one message
				// in the list
				list_add(message_list, m);
				process_post(&receive_message_handler,
							 PROCESS_EVENT_CONTINUE, data);

			}
		}
		//is not active node
		else
		{
			if (inMsg->mode == 1)
			{
				// I'm the new active node, turn on my router and let the previous active node
				// that it can turn of it's router
				actuatorHandler(1);
				prepareMessage(&my_send_message, "", myID, inMsg->srcID, 2, 0);
				sendMessage(unicast_connection, &my_send_message);
				STATUS = 1;
			}
			else if (distance < inMsg->distance)
			{
				// My distance to target is smaller then the active node's, respond to its message
				prepareMessage(&my_send_message, "", myID, inMsg->srcID, 0, distance);
				sendMessage(unicast_connection, &my_send_message);
			}
		}
	}
	PROCESS_END();
}
/*-------------------RECEIVE_MESSAGE_HANDLER-----------------------*/
/* This proccess is here to manipulate the list of mesages received. Since the 
active node broadcasts its distance to target and waits for responses from the other nodes, who verify
their distance is smaller then the active node, therefore this proccess is intended to wait until a timer expires, to collect all the messages and then
iterate through the distances from the ones who responded to identify the one that is the smallest and communicate to that
node that it will be the new active node*/
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
			printf("My distance %d, node distance %d is  %d, min dist %d\n",
				   distance, n->message.srcID, n->message.distance, min_distance);
			if (n->message.distance < min_distance && n->message.distance < distance)
			{
				min_distance = n->message.distance;
				min_distance_p = n;
			}
		}

		if (min_distance_p != NULL)
		{

			printf("Sending message to new active node \n");
			prepareMessage(&my_send_message, "", myID, min_distance_p->message.srcID, 1, 0);
			sendMessage(unicast_connection, &my_send_message);

		}
	}
	PROCESS_END();
}



void actuatorOn(){
	// Here we calculate 10000 numbers of the Fibonnaci sequence in order
	// to simulate the time needed for the actuator to be turned on, which in
	// the case of using a relay circuit to turn on a wireless router for the
	// target to connect, would be the time to activate the relay and measure the 
	// current flowing into the router or verifying that the startup sequence of
	// the router is complete
	int i, n, t1 = 0, t2 = 1, nextTerm;

    for (i = 1; i <= 10000; ++i) {
        printf("%d, ", t1);
        nextTerm = t1 + t2;
        t1 = t2;
        t2 = nextTerm;
    }
	leds_on(LEDS_ALL);
}


void actuatorOff(){
	// Here it is not necessary to simulate the time for the actuator to work
	// since we would just change the voltage value of an analog pin on the board
	// to change the relay state, which would be almost instananeous
	leds_off(LEDS_ALL);
}

void actuatorHandler(int state){
	if(state==0) actuatorOff();
	if(state==1) actuatorOn();	
}

void deleteList()
{
  struct node *n;
  //Cycle through all the nodes to clear
  for (n = list_head(message_list); n != NULL; n = list_item_next(n))
  {

    list_remove(message_list, n);
    memb_free(&message_memb, n);
  }
}