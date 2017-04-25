#include <stddef.h>
#include <ctype.h>
#include "espressif/c_types.h"
#include "lwipopts.h"
#include "lwip/ip_addr.h"
#include "espressif/esp_libc.h"
#include "espressif/esp_misc.h"
#include "espressif/esp_common.h"
#include "espressif/esp_wifi.h"
#include "espressif/esp_sta.h"
#include "espressif/esp_softap.h"
#include "espconn.h"
#include "yeelight.h"

#define YEE_DEBUG

#ifndef YEE_DEBUG
#undef os_printf
#define os_printf
#endif

YeelightConnectionData yeelightConnection;

static const char const sYeeLightSearchCommand[]    = "M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1982\r\nMAN: \"ssdp:discover\"\r\nST: wifi_bulb\r\n";
static const char const sYeeLightMulicastAddress[]  = "239.255.255.250";

const static char * const sYeeLightCommandList[] =
{
    "get_prop",
    "set_ct_abx",
    "set_rgb",
    "set_hsv",
    "set_bright",
    "set_power",
    "toggle",
    "set_default",
    "start_cf",
    "stop_cf",
    "set_scene",
    "cron_add",
    "cron_get",
    "cron_del",
    "set_adjust",
    "set_music",
    "set_name",
};

static const char * const sYeelightProcessSearches[] =
{
    "id:",
    "yeelight:",
    "model:",
    "fw_ver:",
    "support:",
    "power:",
    "bright:",
    "color_mode:",
    "ct:",
    "rgb:",
    "hue:",
    "sat:",
    "name:",
};

const static char * const sYeeLightModelList[] =
{
    "mono",
    "color",
    "stripe",
};

static const char *const sYeelightProperties[] =
{
    "power",
    "bright",
    "ct",
    "rgb",
    "hue",
    "sat",
    "color_mode",
    "flowing",
    "delayoff",
    "flow_params",
    "music_on",
    "name",
};

static int sYLID = 1;

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

void search_packet_init(const char *searchPosition, unsigned short searchLength)
{
    yeelightConnection.searchEnd = searchPosition + searchLength;
    yeelightConnection.searchStart = searchPosition;
}

const char *skip_whitespace(const char *t)
{
    bool done = false;

    while(!done)
    {
        if (!isspace(*t))
        {
            return t;
        }

        t++;
        if (t >= yeelightConnection.searchEnd)
        {
            return NULL;
        }
    }

    return NULL;
}

const char *search_packet(const char * const searchTerm)
{
    const char *t = strstr(yeelightConnection.searchStart, searchTerm);

    if (!t)
    {
        return NULL;
    }

    t += strlen(searchTerm);

    if (t >= yeelightConnection.searchEnd)
    {
        return NULL;
    }

    t = skip_whitespace(t);

    if (!t)
    {
        return NULL;
    }

    return t;
}

// Typical response
// HTTP/1.1 200 OK<CR><CR><LF>
// Cache-Control: max-age=3600<CR><CR><LF>
// Date: <CR><CR><LF>
// Ext: <CR><CR><LF>
// Location: yeelight://192.168.1.118:55443<CR><CR><LF>
// Server: POSIX UPnP/1.0 YGLC/1<CR><CR><LF>
// id: 0x0000000002b07958<CR><CR><LF>
// model: mono<CR><CR><LF>
// fw_ver: 45<CR><CR><LF>
// support: get_prop set_default set_power toggle set_bright start_cf stop_cf set_scene cron_add cron_get cron_del set_adjust set_name<CR><CR><LF>
// power: on<CR><CR><LF>
// bright: 100<CR><CR><LF>
// color_mode: 2<CR><CR><LF>
// ct: 4000<CR><CR><LF>
// rgb: 0<CR><CR><LF>
// hue: 0<CR><CR><LF>
// sat: 0<CR><CR><LF>
// name: <CR><CR><LF>
// <CR><LF>

bool getUnsigned(const char *dataPointer, int *valuePointer, int *bytesProcessedPointer)
{
    bool done = false;
    int value = 0;
    int bytesProcessed = 0;

    while(!done)
    {
        if (dataPointer >= yeelightConnection.searchEnd)
        {
            return false;
        }

        if (isdigit(*dataPointer))
        {
            value *= 10;
            value += (*dataPointer) - '0';
        }
        else
        {
            break;
        }

        dataPointer++;
        bytesProcessed++;

        *valuePointer = value;
        *bytesProcessedPointer = bytesProcessed;
    }    

    return true;
}

int getHexValue(int ascii)
{
    if (isdigit(ascii))
    {
        return ascii - '0';
    }

    if (ascii >= 'a' && ascii <= 'f')
    {
        return ascii - 'a' + 10;
    }

    if (ascii >= 'A' && ascii <= 'F')
    {
        return ascii - 'A' + 10;
    }

    return -1;
}

bool getHex(const char *dataPointer, int32 *hexHighPointer, int32 *hexLowPointer, int *bytesProcessedPointer)
{
    bool done = false;
    uint32 valueHigh = 0;
    uint32 valueLow = 0;
    int bytesProcessed = 0;

    if (dataPointer[0] == '0' && (dataPointer[1] == 'x' || dataPointer[1] == 'X'))
    {
        dataPointer += 2;
    }

    while(!done)
    {
        if (dataPointer >= yeelightConnection.searchEnd)
        {
            return false;
        }

        int hexValue = getHexValue(dataPointer[0]);

        if (hexValue != -1)
        {
            valueHigh <<= 4;
            valueHigh += (valueLow >> 28) & 0xf;
            valueLow <<= 4;
            valueLow += hexValue;
        }
        else
        {
            break;
        }

        dataPointer++;
        bytesProcessed++;

        *hexHighPointer = (int32)valueHigh;
        *hexLowPointer = (int32)valueLow;
        *bytesProcessedPointer = bytesProcessed;
    }    
}

int copyString(const char *source, char *destination, int maxLength)
{
    bool done = false;
    int processedCharacters = 0;

    while (!done)
    {
        if (source >= yeelightConnection.searchEnd)
        {
            return false;
        }

        int c = tolower(*source);

        processedCharacters++;

        if ((c >= 'a' && c <= 'z') || c == '_')
        {
            *destination = *source;

            destination++;

            if (processedCharacters >= maxLength)
            {
                return copyStringCmdTooLong;
            }

            destination[0] = 0; // Always terminate the string
            source++;
        }
        else
        {
            if (c == 0x0d || c == 0x0a)
            {
                return copyStringReturnEnd;
            }

            if (c == ' ')
            {
                return processedCharacters;
            }

            return copyStringInvalidChar;
        }
    }
}

bool process_search_yeelight(const char *dataPointer, YeelightData *yeelightData)
{
    int ipAddress1;
    int ipAddress2;
    int ipAddress3;
    int ipAddress4;
    int port;
    int processed;

    dataPointer += 2; // skip //

    if (!getUnsigned(dataPointer, &ipAddress1, &processed))
    {
        return false;
    }
    dataPointer += processed;

    if (*dataPointer != '.')
    {
        return false;
    }
    dataPointer++;

    if (!getUnsigned(dataPointer, &ipAddress2, &processed))
    {
        return false;
    }
    dataPointer += processed;
    if (*dataPointer != '.')
    {
        return false;
    }
    dataPointer++;

    if (!getUnsigned(dataPointer, &ipAddress3, &processed))
    {
        return false;
    }
    dataPointer += processed;
    if (*dataPointer != '.')
    {
        return false;
    }
    dataPointer++;

    if (!getUnsigned(dataPointer, &ipAddress4, &processed))
    {
        return false;
    }
    dataPointer += processed;
    if (*dataPointer != ':')
    {
        return false;
    }
    dataPointer++;

    if (!getUnsigned(dataPointer, &port, &processed))
    {
        return false;
    }

    IP4_ADDR(&yeelightData->mIPAddress, ipAddress1, ipAddress2, ipAddress3, ipAddress4);
    yeelightData->mPort = port;

    os_printf("%x:%d\n", yeelightData->mIPAddress, yeelightData->mPort);

    return true;
}

bool process_search_id(const char *dataPointer, YeelightData *yeelightData)
{
    int32 hexHigh;
    int32 hexLow;
    int processed;

    if (!getHex(dataPointer, &hexHigh, &hexLow, &processed))
    {
        return false;
    }
    
    yeelightData->mIDHigh = hexHigh;
    yeelightData->mIDLow = hexLow;

    os_printf("id: %x %x\n", yeelightData->mIDHigh, yeelightData->mIDLow);

    return true;
}

bool process_search_model(const char *dataPointer, YeelightData *yeelightData)
{
    char modelType[kSuppportCommandSize];

    int bytesProcessed = copyString(dataPointer, modelType, kSuppportCommandSize);

    if (bytesProcessed == -1 || bytesProcessed == -2)
    {
        return false;
    }

    dataPointer += bytesProcessed;

    int i;

    for(i = 0; i < YLModelCount; i++)
    {
        if (strcmp(modelType, sYeeLightModelList[i]) == 0)
        {
            yeelightData->mModel = i;

           os_printf("model: %d\n", yeelightData->mModel);
            return true;
        }
    }

    return false;
}

bool process_search_firmwareVersion(const char *dataPointer, YeelightData *yeelightData)
{
    int firmwareVersion;
    int processed;

    if (!getUnsigned(dataPointer, &firmwareVersion, &processed))
    {
        return false;
    }

    yeelightData->mFirmwareVersion = firmwareVersion;

    os_printf("fw: %d\n", yeelightData->mFirmwareVersion);

    return true;
}


bool process_search_support(const char *dataPointer, YeelightData *yeelightData)
{
    char supportCommand[kSuppportCommandSize];
    bool done = false;

    while(!done)
    {
        int bytesProcessed = copyString(dataPointer, supportCommand, kSuppportCommandSize);

        if (bytesProcessed == -1 || bytesProcessed == -2)
        {
            return false;
        }

        dataPointer += bytesProcessed;

        int i;

        for(i = 0; i < kYLSupportCount; i++)
        {
            if (strcmp(supportCommand, sYeeLightCommandList[i]) == 0)
            {
                yeelightData->mSupports |= 1 << i;
            }
        }

        if (bytesProcessed == 0)
        {
            os_printf("supported: %x\n", yeelightData->mSupports);
            return true;
        }
    }

    return false;
}

bool process_search_power(const char *dataPointer, YeelightData *yeelightData)
{
    if (toupper(dataPointer[0]) != 'O')
    {
        return false;
    }

    bool power = true;

    if (toupper(dataPointer[1]) == 'F')
    {
        power = false;
    }
    
    yeelightData->mPower = power;

    os_printf("power: %d\n", yeelightData->mPower);

    return true;
}

bool process_search_brightness(const char *dataPointer, YeelightData *yeelightData)
{
    int brightness;
    int processed;

    if (!getUnsigned(dataPointer, &brightness, &processed))
    {
        return false;
    }

    yeelightData->mBrightness = brightness;

    os_printf("br: %d\n", yeelightData->mBrightness);

    return true;
}

bool process_search_colour_mode(const char *dataPointer, YeelightData *yeelightData)
{
    int colourMode;
    int processed;

    if (!getUnsigned(dataPointer, &colourMode, &processed))
    {
        return false;
    }

    yeelightData->mColourMode = colourMode;

    os_printf("cm: %d\n", yeelightData->mColourMode);

    return true;
}

bool process_search_colour_temperature(const char *dataPointer, YeelightData *yeelightData)
{
    int colourTemperature;
    int processed;

    if (!getUnsigned(dataPointer, &colourTemperature, &processed))
    {
        return false;
    }

    yeelightData->mColourTemperature = colourTemperature;

    os_printf("ct: %d\n", yeelightData->mColourTemperature);

    return true;
}

bool process_search_rgb(const char *dataPointer, YeelightData *yeelightData)
{
    int rgb;
    int processed;

    if (!getUnsigned(dataPointer, &rgb, &processed))
    {
        return false;
    }

    yeelightData->mRGB = rgb;

    os_printf("rgb: %x\n", yeelightData->mRGB);

    return true;
}

bool process_search_hue(const char *dataPointer, YeelightData *yeelightData)
{
    int hue;
    int processed;

    if (!getUnsigned(dataPointer, &hue, &processed))
    {
        return false;
    }

    yeelightData->mHue = hue;

    os_printf("hue: %d\n", yeelightData->mHue);

    return true;
}

bool process_search_saturation(const char *dataPointer, YeelightData *yeelightData)
{
    int saturation;
    int processed;

    if (!getUnsigned(dataPointer, &saturation, &processed))
    {
        return false;
    }

    yeelightData->mSaturation = saturation;

    os_printf("sat: %d\n", yeelightData->mSaturation);

    return true;
}

bool process_search_name(const char *dataPointer, YeelightData *yeelightData)
{
    int bytesProcessed = copyString(dataPointer, yeelightData->mAssignedName, kMaxYLName);

    if (bytesProcessed == -1 || bytesProcessed == -2)
    {
        return false;
    }

    os_printf("name: %s\n", yeelightData->mAssignedName);

    return true;
}

static bool (*sYeelightProcessSearchFunctions[])(const char *dataPointer, YeelightData *yeelightData) =
{
    process_search_id,
    process_search_yeelight,
    process_search_model,
    process_search_firmwareVersion,
    process_search_support,
    process_search_power,
    process_search_brightness,
    process_search_colour_mode,
    process_search_colour_temperature,
    process_search_rgb,
    process_search_hue,
    process_search_saturation,
    process_search_name,
};

static bool packetSent = false;

void  udp_recv_callback(void *connectionPtr, char *packetData, unsigned short length)
{
//	struct espconn *this_connection = (struct espconn *)arg;
//	os_printf("udp recv callback: %x\n%s\n", *(int *)this_connection->proto.udp->remote_ip, pdata);
    int i;

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
                yeelightData = (YeelightData *)zalloc(sizeof(YeelightData));
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
    }

    if (yeelightData && !packetSent)
    {
        command_get_prop(&yeelightConnection, yeelightData, kYLPropertyPower | kYLPropertyBrightness);
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

	yeelightData->mESPUDP.local_port = 24000;
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

	err = espconn_send(&yeelightData->mESPConnection, (uint8*)sYeeLightSearchCommand, sizeof(sYeeLightSearchCommand)-1);
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

//typedef void (* espconn_recv_callback)(void *arg, char *pdata, unsigned short len);
//typedef void (* espconn_sent_callback)(void *arg);
//typedef void (* espconn_connect_callback)(void *arg);


void tcp_receive_callback(void *argument, char *dataPointer, unsigned short dataLength)
{
    os_printf("receive: %s\n", dataPointer);
}

void tcp_sent_callback(void *argument)
{
    os_printf("sent\n");
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

char commandString[kMaxCommandString];// = "{\"id\":1,\"method\":\"get_prop\",\"params\":[\"id\",\"power\",\"bright\"]}\r\n";

bool command_get_prop(YeelightConnectionData *yeelightData, YeelightData *bulb, int properties)
{
    int charOffset;

    if (properties == 0)
    {
        return false;
    }

    sprintf(commandString, "{\"id\":%d,\"method\":\"get_prop\",\"params\":[%n", sYLID, &charOffset);

    char *t = commandString + charOffset; // I'm going to pretend that doing it like this is a SIGNIFICANT speed up and not just ugly

    int i;

    bool first = true;

    for (i=0;i<kYLPropertyCount;i++)
    {
        if (properties & (1<<i))
        {
            if (!first)
            {
                *t = ',';
                t++;
            }
            else
            {
                first = false;
            }

            *t = '"';
            t++;
            strcat(t,sYeelightProperties[i]); // Could get length and then memcpy instead, but faster but not nice
            t += strlen(sYeelightProperties[i]);
            *t = '"';
            t++;
        }
    }

    strcat(t,"]}\r\n");

    os_printf("cmd: %s\n", commandString);

    return tcp_send_command(yeelightData, bulb);
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

    tcpConnection->reserve = commandString;

    sint8 error = espconn_connect(tcpConnection);

    if (error == 0)
    {
        return true;
    }

    return false;
}

#define MAX_STACK_SIZE      (512)
#define TASK_PRIORITY       (9)

static const char const sYeeLightTask[] = "yeelight_udp_task";

void yeelight_start()
{
    xTaskCreate(task_udp, sYeeLightTask, MAX_STACK_SIZE, NULL, TASK_PRIORITY, &yeelightConnection.mTaskUDPHandle);

    os_printf("Finally here\n");
}

