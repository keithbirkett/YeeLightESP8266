
#include "yeelight.h"


bool command_get_prop(YeelightConnectionData *yeelightData, YeelightData *bulb, int properties)
{
    int charOffset;

    if (properties == 0)
    {
        return false;
    }

    sprintf(sYeelightCommandString, "{\"id\":%d,\"method\":\"get_prop\",\"params\":[%n", sYeelightID, &charOffset);

    char *t = sYeelightCommandString + charOffset; // I'm going to pretend that doing it like this is a SIGNIFICANT speed up and not just ugly

    int i;

    bool first = true;

    for (i = 0; i < kYLPropertyCount; i++)
    {
        if (properties & (1 << i))
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
            strcat(t, sYeelightProperties[i]); // Could get length and then memcpy instead, faster but not nice
            t += strlen(sYeelightProperties[i]);
            *t = '"';
            t++;
        }
    }

    strcat(t,"]}\r\n");

    os_printf("cmd: %s\n", sYeelightCommandString);

    return tcp_send_command(yeelightData, bulb);
}

#define MAX_STACK_SIZE      (512)
#define TASK_PRIORITY       (9)

static const char const sYeeLightTask[] = "yeelight_udp_task";

void yeelight_start()
{
    xTaskCreate(task_udp, sYeeLightTask, MAX_STACK_SIZE, NULL, TASK_PRIORITY, &yeelightConnection.mTaskUDPHandle);

    os_printf("Finally here\n");
}

