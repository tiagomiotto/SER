#include "servreg-hack.h"
#include "simple-udp.h"

struct Message
{
  char msg[50];
  int id;
};
