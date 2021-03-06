/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#ifndef _DEFINITIONS_H
#define _DEFINITIONS_H
#include "contiki.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib/list.h"
#include "lib/memb.h"
#endif

#include "dev/serial-line.h"

#include "messaging.h"

/*SERVICE ID FOR SYNC NODE IS 1*/
#define UDP_PORT 1234
#define UNICAST_PORT 1235


/* Network State Variables*/
/* This #define defines the maximum amount of neighbors we can remember. */
#define MAX_NODES 255

struct node
{
  /* The ->next pointer is needed since we are placing these on a
     Contiki list. */
  struct node *next;

  /* The ->id field holds the service ID for the node. */
  uint8_t id;

  /* The ->state field holds the state of the node 
    0 - STATE_OFF - ROUTER OVERIDE OFF
    1 - STATE_ON - ROUTER FUNCTIONING NORMALLY
    2 - STATE_ACTIVE - CURRENTLY ACTIVE ROUTER
  */
  uint8_t state;

  int distance;
};
MEMB(nodes_memb, struct node, MAX_NODES);
LIST(nodes_list);

bool updateNodeList_ActiveNode(int nodeID, int state);
void changeNodeSavedState(int nodeID, int state);
void deleteNode(int nodeID);
void deleteList();
void purgeNodeList();
void updateNodesDistances(char* msg);
struct node* searchInList(int nodeID);
void printNetworkInfo();

// volatile int servHackListSize = 0;
// volatile int nodesListSize=0;

#define SEND_INTERVAL (20 * CLOCK_SECOND)

static struct simple_udp_connection unicast_connection;

struct Message my_message;
struct Message messageRx;

volatile int activeNode;

void search_list();

/*---------------------------------------------------------------------------*/

PROCESS(handler_process, "Serial message handler process");
PROCESS(communications_process, "Network size check periodic process");
PROCESS(message_received_handler, "Network size check periodic proce ss");

AUTOSTART_PROCESSES(&communications_process, &handler_process, &message_received_handler);

/*--------------------Communications---------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{

  process_post(&message_received_handler,
               PROCESS_EVENT_CONTINUE, data);
}

/*----------------------------SERIAL HANDLING----------------------------------*/
/* A process to handle the serial input and allow the operator to get the information
on the state of the network and send On/OFF commands to the nodes*/

PROCESS_THREAD(handler_process, ev, data)
{

  PROCESS_BEGIN();

  while (1)
  {
    printf("Use info to get the state of the network or command to send commands\n");
    PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);

    char *msg = (char *)data;

    if (strcmp(msg, "info") == 0)
    {
      printf("The current state of the system is\n");
      printNetworkInfo();
      continue;
    }
    else if (strcmp(msg, "command") == 0)
    {
      printf("Current nodes: ");
      search_list();
      printf("Send the on/off command in the following form: node_number,on\n");
    }
    else
    {
      printf("Invalid option, use info to get the state of the network or command to send commands\n");
      continue;
    }

    PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
    
    msg = (char *)data;
    if (strstr(msg, ",") == NULL)
    {
      printf("Invalid command\n");
      continue;
    }
    char *token = strtok(msg, ",");
    if (token == NULL)
    {
      printf("Invalid command\n");
      continue;
    }
    char *pEnd;
    int destID = strtol(token, &pEnd, 10);
    token = strtok(NULL, ",");

    if (strcmp(token, "on") == 0 || strcmp(token, "off") == 0)
    {

      prepareMessage(&my_message,token,SYNC_NODE_ID, destID, 3, 0);
      sendMessage(unicast_connection,&my_message);
    }
    else
      printf("Invalid command\n");
  }

  PROCESS_END();
}

/*-----------------------------NODE CHECK----------------------------*/
/* A function to get the nodes active in the network*/
void search_list()
{

  struct node *n;

  //Cycle through all the nodes to find the node which changed state.

  for (n = list_head(nodes_list); n != NULL; n = list_item_next(n))
  {
    if (list_item_next(n) != NULL)
      printf("%d, ", n->id);
    else
      printf("%d  \n", n->id);
  }
}

/*-------------------------Unicast Sender Proccess--------------------------------*/
/* A proccess to handle the communications from the sync node to the other nodes*/
PROCESS_THREAD(communications_process, ev, data)
{

  PROCESS_BEGIN();

  servreg_hack_init();
  simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);

  registerConnection(SYNC_NODE_ID);

  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
  }

  PROCESS_END();
}
/* A process to handle the received messages from the nodes and act accordingly*/
PROCESS_THREAD(message_received_handler, ev, data)
{

  PROCESS_BEGIN();

  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
    struct Message *inMsg = (struct Message *)data;
    messageRx = *inMsg;
    char *pEnd;

    // Message received when the active node changes
    if (messageRx.mode == 4)
    {

      updateNodeList_ActiveNode(messageRx.srcID, STATE_ACTIVE);
      printf("New state of the network \n");
      printNetworkInfo();
      //updateNodesDistances(messageRx.msg);

    }
    // Message received as a response to an ON/OFF command
    else if (messageRx.mode == 3)
    {
      
      if(strcmp(messageRx.data,"on")==0) changeNodeSavedState(messageRx.srcID, STATE_ON);
      if(strcmp(messageRx.data,"off")==0) changeNodeSavedState(messageRx.srcID, STATE_OFF);
    }
  }

  PROCESS_END();
}

// Function that updates the nodes list and sets the active node
bool updateNodeList_ActiveNode(int nodeID, int state)
{
  servreg_hack_item_t *item;
  struct node *n;

  // if (nodeID == activeNode)
  //   return false;
  //Cycle through all the nodes to update the list, adding those missing and updating the node
  for (item = servreg_hack_list_head();
       item != NULL;
       item = list_item_next(item))
  {

    int serviceID = servreg_hack_item_id(item);

    //Don't add the sync node
    if (serviceID == 1)
      continue;

    // Check if we already know this neighbor.
    for (n = list_head(nodes_list); n != NULL; n = list_item_next(n))
    {

      // We break out of the loop if the address of the noode already exists in the list
      if (n->id == serviceID)
      {

        // If this is the new active node update status
        if (serviceID == nodeID)
          n->state = state;

        // If it is not the new active node, change its state
        else if (n->state == STATE_ACTIVE)
          n->state = STATE_ON;

        break;
      }
    }

    // If the node was not found, add it to the list
    if (n == NULL)
    {
      n = memb_alloc(&nodes_memb);

      // Initialize the fields.
      n->id = serviceID;
      if (serviceID == nodeID)
        n->state = STATE_ACTIVE;
      else
        n->state = STATE_ON;

      // Place the node on the node list.
      list_add(nodes_list, n);
    }
  }
  purgeNodeList();
}

/* Function that updates the state(ON/OFF) of the node that received the command*/
void changeNodeSavedState(int nodeID, int state)
{
  struct node *n;

  //Cycle through all the nodes to find the node which changed state.
  for (n = list_head(nodes_list); n != NULL; n = list_item_next(n))
  {

    // We break out of the loop if the address of the noode already exists in the list
    if (n->id == nodeID)
    {
      n->state = state;
      break;
    }
  }
}

/* Simple function to find a node in the list*/
struct node* searchInList(int nodeID){
  struct node *n;
  //Cycle through all the nodes to find the node which changed state.
  for (n = list_head(nodes_list); n != NULL; n = list_item_next(n))
  {

    // We break out of the loop if the address of the noode already exists in the list
    if (n->id == nodeID)
    {
      return n;
    }
  }
  return NULL;
}
/* Simple function to delete a node from the list when it is no longer active*/
void deleteNode(int nodeID)
{
  struct node *n;
  n = searchInList(nodeID);
  list_remove(nodes_list, n);
  memb_free(&nodes_memb, n);
}

/* Simple function to delete the entire list and free memory*/
void deleteList()
{
  struct node *n;
  //Cycle through all the nodes to find the node which changed state.
  for (n = list_head(nodes_list); n != NULL; n = list_item_next(n))
  {

    list_remove(nodes_list, n);
    memb_free(&nodes_memb, n);
  }
}

/* Function to purge the node list, by checking which ones are still reachable and removing,
those who are not*/
void purgeNodeList(){
    
  struct node *n;
 
  uip_ipaddr_t *addr;
    
  //Cycle through all the nodes to find the node which changed state.
  for (n = list_head(nodes_list); n != NULL; n = list_item_next(n))
  {
  addr = servreg_hack_lookup(n->id);
  if(addr==NULL) {
    list_remove(nodes_list, n);
    memb_free(&nodes_memb, n);
  }
  }
  
}

/* Function to update the distance of the nodes
The purpouse of this function was to expand on the information kept on the network,
but in the end it wasn't implemented*/
void updateNodesDistances(char* msg){
  char *token;
  char *token2;
  char *pEnd;
  struct node *n;
 
   /* get the first token */
  token = strtok(msg, ";");

  while(token != NULL){

    token2 = strtok(token,",");
    int destID = strtol(token2, &pEnd, 10);
    token2 = strtok(NULL, ",");
    int dist = strtol(token2, &pEnd, 10);
    n= searchInList(destID);
    n->distance=dist;

    token = strtok(NULL, ";");
  }
}

/* Print the network state for the info command*/
void printNetworkInfo(){
   struct node *n;
  for (n = list_head(nodes_list); n != NULL; n = list_item_next(n))
  {

      printf("%d | ", n->id);
      if(n->state== STATE_ACTIVE) printf("ACTIVE \n");
      if(n->state== STATE_ON) printf("ON \n"); 
      if(n->state== STATE_OFF) printf("OFF \n");

  }

}