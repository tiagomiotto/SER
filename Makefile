all: broadcast-example unicast-sender unicast-receiver
APPS=servreg-hack
CONTIKI=/home/user/Desktop/contiki

CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
