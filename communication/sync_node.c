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

#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"


#include "dev/serial-line.h"
#include "net/rpl/rpl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "messaging.h"

/*SERVICE ID FOR SYNC NODE IS 1*/
#define UDP_PORT 1234
#define UNICAST_PORT 1235
#define ID 1

#define SEND_INTERVAL (20 * CLOCK_SECOND)

static struct simple_udp_connection unicast_connection;

struct message
{
  char msg[50];
  int id;
};

struct Message my_message;

void search_list();

/*---------------------------------------------------------------------------*/

PROCESS(handler_process, "Serial message handler process");
PROCESS(serial_process, "Serial line test process");
PROCESS(unicast_sender_process, "Network size check periodic process");

AUTOSTART_PROCESSES( &unicast_sender_process, &serial_process, &handler_process /*, &available_nodes_proccess*/);

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
}

static uip_ipaddr_t *set_global_address(void)
{
  static uip_ipaddr_t ipaddr;
  int i;
  uint8_t state;

  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

  printf("IPv6 addresses: ");
  for (i = 0; i < UIP_DS6_ADDR_NB; i++)
  {
    state = uip_ds6_if.addr_list[i].state;
    if (uip_ds6_if.addr_list[i].isused &&
        (state == ADDR_TENTATIVE || state == ADDR_PREFERRED))
    {
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      printf("\n");
    }
  }

  return &ipaddr;
}

static void create_rpl_dag(uip_ipaddr_t *ipaddr)
{
  struct uip_ds6_addr *root_if;

  root_if = uip_ds6_addr_lookup(ipaddr);
  if (root_if != NULL)
  {
    rpl_dag_t *dag;
    uip_ipaddr_t prefix;

    rpl_set_root(RPL_DEFAULT_INSTANCE, ipaddr);
    dag = rpl_get_any_dag();
    uip_ip6addr(&prefix, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
    rpl_set_prefix(dag, &prefix, 64);
    PRINTF("created a new RPL dag\n");
  }
  else
  {
    PRINTF("failed to create a new RPL DAG\n");
  }
}

void send_command(char* messageTX, uint8_t SERVICE_ID)
{

    strcpy(my_message.msg, messageTX);
    //my_message.id=SERVICE_ID;
    process_post(&unicast_sender_process,
                 PROCESS_EVENT_CONTINUE, &my_message);
  

}
/*---------------------------------------------------------------------------*/

/*----------------------------SERIAL HANDLING----------------------------------*/
PROCESS_THREAD(handler_process, ev, data)
{

  int i = 0;
  
  PROCESS_BEGIN();

  while (1)
  {
      printf("Use info to get the state of the network or command to send commands\n");
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);

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

    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
    //Verificar aqui, se madno uma coisa que não é um numero ele morre
    msg = (char *)data;
    char *token = strtok(msg, ",");
    if(token == NULL){
      printf("Invalid command\n");
      continue;
    }
    char *pEnd;
    int destID = strtol(token, &pEnd, 10);
    token = strtok(NULL,",");
    

    if(strcmp(token,"on") ==0|| strcmp(token,"off")==0){
      //send_command(token,id);
      my_message = prepareMessage(token,ID,destID,1);
      process_post(&unicast_sender_process,
                 PROCESS_EVENT_CONTINUE, &my_message);
      
    }
    else printf("Invalid command\n");

  }

  PROCESS_END();
}

PROCESS_THREAD(serial_process, ev, data)
{
  PROCESS_BEGIN();

  for (;;)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
    process_post_synch(&handler_process,
                 PROCESS_EVENT_CONTINUE, data);
  }
  PROCESS_END();
}

/*-----------------------------NODE CHECK----------------------------*/
void search_list()
{
  servreg_hack_item_t *item;
  for (item = servreg_hack_list_head();
       item != NULL;
       item = list_item_next(item))
  {
    if(list_item_next(item)!= NULL)
    printf("%d, ", servreg_hack_item_id(item));
    else
    printf("%d\n",servreg_hack_item_id(item));


  }
}

/*-------------------------Unicast Sender Proccess--------------------------------*/

PROCESS_THREAD(unicast_sender_process, ev, data)
{
  uip_ipaddr_t *addr;
  uip_ipaddr_t *ipaddr;
  PROCESS_BEGIN();

  servreg_hack_init();
  
  ipaddr = set_global_address();



  servreg_hack_register(ID, ipaddr);

  simple_udp_register(&unicast_connection, UDP_PORT,
                      NULL, UDP_PORT, receiver);


  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);

    struct Message *my_messageRX = data;
    sendMessage(unicast_connection,my_messageRX);
    // addr = servreg_hack_lookup(my_messageRX->id);
    // if (addr != NULL)
    // {

    //   my_message.id=ID;
    //   printf("Sending unicast to ");
    //   uip_debug_ipaddr_print(addr);
    //   printf("\n");
    //   simple_udp_sendto(&unicast_connection, &my_message, sizeof(struct message) + 1, addr);
    // }
    // else
    // {
    //   printf("Service %d not found\n", my_messageRX->id);
    // }
  }

  PROCESS_END();
}