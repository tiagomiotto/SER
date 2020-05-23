#include "servreg-hack.h"
#include "simple-udp.h"

struct Message
{
  char msg[50];
  int id;
};

void sendMessage(simple_udp_connection connection,
                  struct Message *messageTX);

struct Message prepareMessage(char* data, uint8_t txID, uint8_t code);