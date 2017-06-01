#include "yeelight.h"

static os_timer_t sTimer;

#define TCP_DISCONNECT_CALLBACK (1000)

void tcp_receive_callback(void *argument, char *dataPointer, unsigned short dataLength)
{
    os_printf("receive: %s\n", dataPointer);
}

void tcp_disconnect_timeout(void *pTimerArg)
{
    struct espconn *connection = pTimerArg;

    sint8 error = espconn_disconnect(connection);

    os_printf("tcp_disconnect_timeout: %d %p\n", error, connection);
}

void tcp_sent_callback(void *argument)
{
    os_printf("sent\nsetting callback\n");
    os_timer_disarm(&sTimer);
    os_timer_setfn(&sTimer, (os_timer_func_t *)tcp_disconnect_timeout, argument);
    os_timer_arm(&sTimer, TCP_DISCONNECT_CALLBACK, 0);
}

void tcp_disconnect_callback(void *argument)
{
    os_printf("disconnect\n");
}

void tcp_connect_callback(void *argument)
{
    struct espconn *connection = argument;

    os_printf("connect\n");

    espconn_regist_recvcb(connection, tcp_receive_callback);
    espconn_regist_sentcb(connection, tcp_sent_callback);
    espconn_regist_disconcb(connection, tcp_disconnect_callback);

    sint8 error = espconn_sent(connection, connection->reserve, strlen(connection->reserve));

    os_printf("tcp_send:sent: %d:%p:%d\n", error, connection->reserve, strlen(connection->reserve));
}

void tcp_reconnect_callback(void *argument, sint8 error)
{
    os_printf("re-connect: %d\n", error);
}

bool tcp_send_command(YeelightConnectionData *yeelightData, YeelightData *bulb)
{
    struct espconn  *tcpConnection = &yeelightData->mESPConnectionTCP;

   	memset(tcpConnection, 0, sizeof(struct espconn));
	memset(&yeelightData->mESPTCP, 0, sizeof(esp_tcp));

	tcpConnection->type = ESPCONN_TCP;
	tcpConnection->state = ESPCONN_NONE;
	tcpConnection->proto.tcp = &yeelightData->mESPTCP;

    IPADDR2_COPY(tcpConnection->proto.tcp->remote_ip, &bulb->mIPAddress);
    tcpConnection->proto.tcp->remote_port = bulb->mPort;
    tcpConnection->proto.tcp->local_port = espconn_port();

    espconn_regist_connectcb(tcpConnection, tcp_connect_callback); // register connect callback
    espconn_regist_reconcb(tcpConnection, tcp_reconnect_callback); // register reconnect callback as error handler

    tcpConnection->reserve = sYeelightCommandString;

    sint8 error = espconn_connect(tcpConnection);

    os_printf("Sending command: %d %s %p\n", error, sYeelightCommandString, tcpConnection);

    sYeelightCommandID++;

    if (error == 0)
    {
        return true;
    }

    return false;
}