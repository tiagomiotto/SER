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
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"
#include "servreg-hack.h"

#include "simple-udp.h"
#include "dev/serial-line.h"
#include "net/rpl/rpl.h"

#include <stdio.h>
#include <string.h>

#define UDP_PORT 1234
#define SERVICE_ID 1

/*SERVICE ID FOR SYNC NODE IS 1*/
#define SEND_INTERVAL (20 * CLOCK_SECOND)

static struct simple_udp_connection broadcast_connection;

/*---------------------------------------------------------------------------*/
PROCESS(communications_process, "UDP broadcast example process");
PROCESS(handler_process, "Serial message handler process");
PROCESS(serial_process, "Serial line test process");
PROCESS(available_nodes_proccess, "Network size check periodic process");

AUTOSTART_PROCESSES(&communications_process, &serial_process, &handler_process, &available_nodes_proccess);

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

void send_command(uint8_t SERVICE_ID)
{

  addr = servreg_hack_lookup(SERVICE_ID);
  if (addr != NULL)
  {
    static unsigned int message_number;
    char buf[20];

    printf("Sending unicast to ");
    uip_debug_ipaddr_print(addr);
    printf("\n");
    sprintf(buf, "Message %d", message_number);
    message_number++;
    simple_udp_sendto(&unicast_connection, buf, strlen(buf) + 1, addr);
  }
  else
  {
    printf("Service %d not found\n", SERVICE_ID);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(communications_process, ev, data)
{

  uip_ipaddr_t addr;
  uip_ipaddr_t *ipaddr;
  PROCESS_BEGIN();

  simple_udp_register(&broadcast_connection, UDP_PORT,
                      NULL, UDP_PORT,
                      receiver);

  servreg_hack_init();

  ipaddr = set_global_address();

  create_rpl_dag(ipaddr);

  servreg_hack_register(SERVICE_ID, ipaddr);

  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev = PROCESS_EVENT_CONTINUE);
    char *msg = (char *)data;

    printf("Sending broadcast %s\n", msg);
    uip_create_linklocal_allnodes_mcast(&addr);
    simple_udp_sendto(&broadcast_connection, msg, 5, &addr);
  }

  PROCESS_END();
}

/*----------------------------SERIAL HANDLING----------------------------------*/
PROCESS_THREAD(handler_process, ev, data)
{

  int i = 0;
  PROCESS_BEGIN();

  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);

    char *msg = (char *)data;

    if (strcmp(msg, "info") == 0)
    {
      printf("The current state of the system is %s\n", (char *)msg);
      continue;
    }
    else if (strcmp(msg, "command") == 0)
    {
      printf("The current nodes are ");
      for (i = 0; i < 4; i++)
      {
        printf("%d, ", i);
      }
      printf("select on/off followed by the number of the node\n");
    }
    else
    {
      printf("Invalid option, use info to get the state of the networrk or command to send commands\n");
      continue;
    }

    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
    msg = (char *)data;

    if (strcmp(msg, "c") == 0)
      printf("it works1 %s \n", (char *)msg);
    else if (strcmp(msg, "d") == 0)
      printf("it works2 %s \n", (char *)msg);
    else
    {
      printf("Invalid option\n");
      continue;
    }
  }

  PROCESS_END();
}

PROCESS_THREAD(serial_process, ev, data)
{
  PROCESS_BEGIN();
  printf("Use info to get the state of the network or command to send commands\n");
  for (;;)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
    process_post(&handler_process,
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
    printf("Id %d address ", servreg_hack_item_id(item));
    send_command(servreg_hack_item_id(item));
    uip_debug_ipaddr_print(servreg_hack_item_address(item));
    printf("\n");
  }
}
PROCESS_THREAD(available_nodes_proccess, ev, data)
{
  static struct etimer periodic_timer;
  PROCESS_BEGIN();
  servreg_hack_init();

  etimer_set(&periodic_timer, SEND_INTERVAL);
  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);

    printf("Check nodes \n");

    search_list();
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
