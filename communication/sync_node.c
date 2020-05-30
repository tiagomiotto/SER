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
#define ID 1

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
};
MEMB(nodes_memb, struct node, MAX_NODES);
LIST(nodes_list);

bool updateNodeList_ActiveNode(int nodeID, int state);
void changeNodeSavedState(int nodeID, int state);

#define SEND_INTERVAL (20 * CLOCK_SECOND)

static struct simple_udp_connection unicast_connection;

struct Message my_message;

volatile int activeNode =0;

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
  printf("Data received on port %d from port %d with length %d : %s\n",
         receiver_port, sender_port, datalen, (char *)data);
  process_post(&message_received_handler,
               PROCESS_EVENT_CONTINUE, data);
}

/*----------------------------SERIAL HANDLING----------------------------------*/
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
      printf("The current state of the system is %s\n", (char *)msg);
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
    //Verificar aqui, se madno uma coisa que não é um numero ele morre
    msg = (char *)data;
    if(strstr(msg, ",") == NULL) {
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

      my_message = prepareMessage(token, ID, destID, 1);
      process_post(&communications_process,
                   PROCESS_EVENT_CONTINUE, &my_message);
    }
    else
      printf("Invalid command\n");
  }

  PROCESS_END();
}

/*-----------------------------NODE CHECK----------------------------*/
void search_list()
{

  struct node *n;

  //Cycle through all the nodes to find the node which changed state.

  for (n = list_head(nodes_list); n != NULL; n = list_item_next(n))
  {
    if (list_item_next(n) != NULL)
      printf("%d | %d, ", n->id, n->state);
      else printf("%d | %d \n", n->id, n->state);
  }

}

/*-------------------------Unicast Sender Proccess--------------------------------*/

PROCESS_THREAD(communications_process, ev, data)
{

  PROCESS_BEGIN();

  servreg_hack_init();
  simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);

  registerConnection(ID);

  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
    struct Message *my_messageRX = data;
    sendMessage(unicast_connection, my_messageRX);
  }

  PROCESS_END();
}

PROCESS_THREAD(message_received_handler, ev, data)
{

  PROCESS_BEGIN();

  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
    char *msg = (char *)data;
    char *token = strtok(msg, ",");
    char *pEnd;
    int nodeID = strtol(token, &pEnd, 10);
    token = strtok(NULL, ",");
    int state = strtol(token, &pEnd, 10);
    printf("%d,%d\n", nodeID,state);
    if(state == STATE_ACTIVE) { 
      printf("Update active node\n");
      updateNodeList_ActiveNode(nodeID, state);
    }

    else if(state == STATE_ON || state == STATE_OFF) {
      printf("Update on node\n"); 
      changeNodeSavedState(nodeID, state);
    }
  }

  PROCESS_END();
}

bool updateNodeList_ActiveNode(int nodeID, int state)
{
  servreg_hack_item_t *item;
  struct node *n;

  if(nodeID==activeNode) return false;
  //Cycle through all the nodes to update the list, adding those missing and updating the node
  for (item = servreg_hack_list_head();
       item != NULL;
       item = list_item_next(item))
  {
    
    int serviceID = servreg_hack_item_id(item);
    
    //Don't add the sync node
    if(serviceID==1) continue;

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
      if(serviceID == nodeID) n->state = STATE_ACTIVE;
      else n->state = STATE_ON;
  

      // Place the node on the node list.
      list_add(nodes_list, n);
    }
  }
}

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