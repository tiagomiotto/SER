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

#include "simple-udp.h"
#include "dev/serial-line.h"

#include <stdio.h>
#include <string.h>

#define UDP_PORT 1234

#define SEND_INTERVAL (20 * CLOCK_SECOND)
#define SEND_TIME (random_rand() % (SEND_INTERVAL))

static struct simple_udp_connection broadcast_connection;

/*---------------------------------------------------------------------------*/
PROCESS(broadcast_example_process, "UDP broadcast example process");
PROCESS(handler_process, "Serial message handler process");
PROCESS(test_serial, "Serial line test process");
PROCESS(available_nodes_proccess, "Network size check periodic process");

AUTOSTART_PROCESSES(&test_serial, &handler_process, &available_nodes_proccess);

/*---------------------------------------------------------------------------*/
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
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_example_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer send_timer;
  uip_ipaddr_t addr;

  PROCESS_BEGIN();

  simple_udp_register(&broadcast_connection, UDP_PORT,
                      NULL, UDP_PORT,
                      receiver);

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

PROCESS_THREAD(handler_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer send_timer;
  uip_ipaddr_t addr;
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
  //servreg_hack_item_t *servreg_hack_list_head(void);
}

PROCESS_THREAD(test_serial, ev, data)
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


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(available_nodes_proccess, ev, data)
{
  static struct etimer periodic_timer;
  uip_ipaddr_t addr;

  PROCESS_BEGIN();

  etimer_set(&periodic_timer, SEND_INTERVAL);
  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
    
    printf("Check nodes\n", msg);

  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
