#include "yeelight.h"

static bool packetSent = false;

void join_igmp()
{
    struct ip_addr ipgroup;
    struct ip_info local_ip;

    const WIFI_MODE mode = wifi_get_opmode();

    if (mode & STATION_MODE)
    {
        wifi_get_ip_info(STATION_IF, &local_ip);
    }
    else
    {
        wifi_get_ip_info(SOFTAP_IF, &local_ip);
    }

    ipaddr_aton(sYeeLightMulicastAddress, &ipgroup);

    const uint8_t iret = igmp_joingroup(&local_ip.ip, (struct ip_addr *)(&ipgroup));

    if (iret != 0)
    {
        os_printf("Could not join\n");
    }
}

void  udp_recv_callback(void *connectionPtr, char *packetData, unsigned short length)
{
	struct espconn *this_connection = (struct espconn *)connectionPtr;
	os_printf("udp recv callback: %x\n", *(int *)this_connection->proto.udp->remote_ip);
    int i;
    YeelightID yeelightID;

    search_packet_init(packetData, length);

    YeelightData *yeelightData = NULL;
    for(i = 0; i < kYLProcessSearchCount; i++)
    {
        const char *t = search_packet(sYeelightProcessSearches[i]);

        //os_printf("%s:%s\n",sYeelightProcessSearches[i],t);

        if (i == kYLProcessSearchID)
        {
            if (t == NULL)
            {
                // No lights found
                break;
            }
            else
            {
                yeelightData = (YeelightData *)&yeelightID; // Fake storage just to pick up the ID without using too much stack space
            }
        }
        else
        {
            if (t == NULL)
            {
                continue;
            }
        }

        sYeelightProcessSearchFunctions[i](t, yeelightData);

        if (i == kYLProcessSearchID)
        {
            yeelightData = list_search_light(yeelightData->mIDHigh, yeelightData->mIDLow);

            if (yeelightData == NULL)
            {
                yeelightData = (YeelightData *)zalloc(sizeof(YeelightData));

                yeelightData->mIDHigh = yeelightID.mIDHigh;
                yeelightData->mIDLow = yeelightID.mIDLow;

                yeelightData->mLocalID = sYeelightNextID;
                sYeelightNextID++;

                list_add(yeelightData);
                os_printf("Light not found adding new.\n");
            }
            else
            {
                os_printf("Light found, using previous.\n");
            }
        }
    }

    list_lights();

    // Test code
    if (yeelightData && !packetSent)
    {
        //command_set_adjust(&yeelightConnection, yeelightData, kTLAdjustCircle, kTLAdjustColour);
        packetSent = true;
    }
}

void  udp_send_callback(void *arg)
{
	struct espconn *this_connection = (struct espconn *)arg;

	os_printf("udp sent callback: %d.%d.%d.%d:%d\n", this_connection->proto.udp->remote_ip[0]
	, this_connection->proto.udp->remote_ip[1]
	, this_connection->proto.udp->remote_ip[2]
	, this_connection->proto.udp->remote_ip[3],
	this_connection->proto.udp->remote_port);
	os_printf("udp sent callback: %d.%d.%d.%d:%d\n", this_connection->proto.udp->local_ip[0]
	, this_connection->proto.udp->local_ip[1]
	, this_connection->proto.udp->local_ip[2]
	, this_connection->proto.udp->local_ip[3],
	this_connection->proto.udp->local_port);
}


void task_udp_create(YeelightConnectionData *yeelightData)
{
    struct ip_info ipconfig;
	sint8 err;

	memset(&yeelightData->mESPConnection, 0, sizeof(struct espconn));
	memset(&yeelightData->mESPUDP, 0, sizeof(esp_udp));

    wifi_get_ip_info(STATION_IF, &ipconfig);

	yeelightData->mESPConnection.type = ESPCONN_UDP;
	yeelightData->mESPConnection.state = ESPCONN_NONE;
	yeelightData->mESPConnection.proto.udp = &yeelightData->mESPUDP;

	yeelightData->mESPUDP.remote_port = 1982;
	IP4_ADDR((ip_addr_t *)yeelightData->mESPConnection.proto.udp->remote_ip, 239, 255, 255, 250);
	//IP4_ADDR((ip_addr_t *)yeelightData->mESPConnection.proto.udp->remote_ip, 192, 168, 1, 72);

	yeelightData->mESPUDP.local_port = 1982;
	*(uint32 *)yeelightData->mESPUDP.local_ip = ipconfig.ip.addr;

	err = espconn_create(&yeelightData->mESPConnection);
	os_printf("err1: %d\n",err);
	err = espconn_regist_recvcb(&yeelightData->mESPConnection, udp_recv_callback);
	os_printf("err2: %d\n",err);
	err = espconn_regist_sentcb(&yeelightData->mESPConnection, udp_send_callback);
	os_printf("err2a: %d\n",err);
}

void task_udp_send(YeelightConnectionData *yeelightData)
{
	sint8 err;

	err = espconn_send(&yeelightData->mESPConnection, (uint8*)sYeeLightSearchCommand, yeelight_search_command_size());
	os_printf("err3: %d\n",err);
}

void task_udp(void *pvParameters)
{
    os_printf("In task\n");

    int phase = 0;

    while (1)
    {
        vTaskDelay(500 / portTICK_RATE_MS);

        switch (phase)
        {
            case 0:
                join_igmp();
                task_udp_create(&yeelightConnection);
                task_udp_send(&yeelightConnection);
                phase++;
            break;
        }

        //os_printf("Task run: %d\n", phase);
    }
}
