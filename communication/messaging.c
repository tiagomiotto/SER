#include "messaging.h"

void sendMessage(struct simple_udp_connection connection,
                 struct Message *messageTX)
{
    uip_ipaddr_t *addr;
    addr = servreg_hack_lookup(messageTX->destID);
    if (addr != NULL)
    {

        printf("Sending unicast to ");
        uip_debug_ipaddr_print(addr);
        printf(" %s, %d, %d, %d\n", messageTX->msg, messageTX->destID, messageTX->srcID, messageTX->code);
        simple_udp_sendto(&connection, messageTX, sizeof(struct Message) + 1, addr);
    }
    else
    {
        printf("Service %d not found\n", messageTX->destID);
    }
}

void sendStateToSync(struct simple_udp_connection connection, int mydID, int state)
{
    uip_ipaddr_t *addr;
    addr = servreg_hack_lookup(1);
    if (addr != NULL)
    {
        printf("Sending state change to sync node | State: %d\n", state);
        char buf[20];
        sprintf(buf, "%d,%d", mydID, state);
        simple_udp_sendto(&connection, buf, sizeof(buf) + 1, addr);
    }
    else
    {
        printf("Sync node unreacheable\n");
    }
}

struct Message prepareMessage(char *data, uint8_t srcID, uint8_t destID, uint8_t code)
{
    struct Message messageTx;
    strcpy(messageTx.msg, data);
    messageTx.srcID = srcID;
    messageTx.destID = destID;
    messageTx.code = code;
    return messageTx;
}

uip_ipaddr_t *set_global_address(void)
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

void create_rpl_dag(uip_ipaddr_t *ipaddr)
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
        printf("created a new RPL dag\n");
    }
    else
    {
        printf("failed to create a new RPL DAG\n");
    }
}

uint8_t generateID()
{
    servreg_hack_item_t *item;
    uint8_t max = 0;
    for (item = servreg_hack_list_head();
         item != NULL;
         item = list_item_next(item))
    {

        if (servreg_hack_item_id(item) > max)
            max = servreg_hack_item_id(item);
    }
    if (max > 1)
        return max + 1;
    return 190;
}

int registerConnection(uint8_t ID)
{
    uip_ipaddr_t *ipaddr;

    ipaddr = set_global_address();

    //For receivers assingn an ID and if it is the sync node start the RPL
    if (ID == 1)
    {

        create_rpl_dag(ipaddr);
    }
    else
        ID = generateID();
    printf("My ID is: %d\n", ID);
    servreg_hack_register(ID, ipaddr);

    return ID;
}