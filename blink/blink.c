/**
 * \file
 *         A very simple Contiki application for making blinking leds
 * \author
 *         Vasco Rato
 */

#include "contiki.h"
#include "dev/leds.h"
#include <stdio.h> /* For printf() */
/*---------------------------------------------------------------------------*/
PROCESS(blink_process, "Blink process");
AUTOSTART_PROCESSES(&blink_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(blink_process, ev, data)
{
  PROCESS_BEGIN();
  leds_on(LEDS_ALL);
  
  static uint32_t seconds_green = 1;
  static uint32_t seconds_yellow = 2;
  static uint32_t seconds_red = 4;
  static struct etimer et_green; //define the timers
  static struct etimer et_yellow;
  static struct etimer et_red;

  etimer_set(&et_green, CLOCK_SECOND*seconds_green);  // Set the timers
  etimer_set(&et_yellow, CLOCK_SECOND*seconds_yellow);
  etimer_set(&et_red, CLOCK_SECOND*seconds_red);

  printf("\tSTART\r\n");


  while(1){
    PROCESS_WAIT_EVENT();
      
    if(etimer_expired(&et_green)) {  // If the event it's provoked by the timer expiration, then...
      leds_toggle(LEDS_GREEN);	
      etimer_reset(&et_green);
    }
    if(etimer_expired(&et_yellow)) {
 	 printf("Hello, world\n");
      leds_toggle(LEDS_YELLOW);	
      etimer_reset(&et_yellow);
    }
    if(etimer_expired(&et_red)) {  
      leds_toggle(LEDS_RED);	
      etimer_reset(&et_red);
    }	
  }		
  exit:
    leds_off(LEDS_ALL);
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
