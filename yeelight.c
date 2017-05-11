
#include "yeelight.h"

void display_command()
{
    os_printf("cmd: %s\n", sYeelightCommandString);
}

bool set_command_with_value(YeelightConnectionData *yeelightData, YeelightData *bulb, YeelightCommandID command, int value, YeelightEffectType effect, int duration)
{
    sprintf(sYeelightCommandString, "{\"id\":%d,\"method\":\"%s\",\"params\":[%d,\"%s\",%d]}\r\n",
        sYeelightCommandID, sYeelightCommandList[command], value, sYeelightEffectType[effect], duration);

    display_command();

    return tcp_send_command(yeelightData, bulb);
}

bool command_set_ct_abx(YeelightConnectionData *yeelightData, YeelightData *bulb, int value, YeelightEffectType effect, int duration)
{
    return set_command_with_value(yeelightData, bulb, kYLSupportSetCTAbx, value, effect, duration);
}

bool command_set_rgb(YeelightConnectionData *yeelightData, YeelightData *bulb, int value, YeelightEffectType effect, int duration)
{
    return set_command_with_value(yeelightData, bulb, kYLSupportSetRGB, value, effect, duration);
}

bool command_set_hsv(YeelightConnectionData *yeelightData, YeelightData *bulb, int value, YeelightEffectType effect, int duration)
{
    return set_command_with_value(yeelightData, bulb, kYLSupportSetHSV, value, effect, duration);
}

bool command_set_bright(YeelightConnectionData *yeelightData, YeelightData *bulb, int value, YeelightEffectType effect, int duration)
{
    return set_command_with_value(yeelightData, bulb, kYLSupportSetBright, value, effect, duration);
}

bool command_set_power(YeelightConnectionData *yeelightData, YeelightData *bulb, bool on, YeelightEffectType effect, int duration)
{
    sprintf(sYeelightCommandString, "{\"id\":%d,\"method\":\"%s\",\"params\":[\"%s\",\"%s\",%d]}\r\n",
        sYeelightCommandID, sYeelightCommandList[kYLSupportSetPower], on ? "on" : "off", sYeelightEffectType[effect], duration);

    display_command();

    return tcp_send_command(yeelightData, bulb);
}

bool command_set_toggle(YeelightConnectionData *yeelightData, YeelightData *bulb)
{
    sprintf(sYeelightCommandString, "{\"id\":%d,\"method\":\"%s\",\"params\":[]}\r\n",
        sYeelightCommandID, sYeelightCommandList[kYLSupportToggle]);

    display_command();

    return tcp_send_command(yeelightData, bulb);
}

bool command_set_default(YeelightConnectionData *yeelightData, YeelightData *bulb)
{
    sprintf(sYeelightCommandString, "{\"id\":%d,\"method\":\"%s\",\"params\":[]}\r\n",
        sYeelightCommandID, sYeelightCommandList[kYLSupportSetDefault]);

    display_command();

    return tcp_send_command(yeelightData, bulb);
}

static const char * const sYeelightAdjustAction[] = 
{
    "increase",
    "decrease",
    "circle",
};

static const char * const sYeelightAdjustProperty[] = 
{
    "bright",
    "ct",
    "color",
};

bool command_set_adjust(YeelightConnectionData *yeelightData, YeelightData *bulb, YeelightAdjustAction action, YeelightAdjustProp property)
{
    if (property == kTLAdjustColour)
    {
        action = kTLAdjustCircle;
    }

    sprintf(sYeelightCommandString, "{\"id\":%d,\"method\":\"%s\",\"params\":[\"%s\",\"%s\"]}\r\n",
        sYeelightCommandID, sYeelightCommandList[kYLSupportSetAdjust], sYeelightAdjustAction[action], sYeelightAdjustProperty[property]);

    display_command();

    return tcp_send_command(yeelightData, bulb);
}

bool command_get_prop(YeelightConnectionData *yeelightData, YeelightData *bulb, int properties)
{
    int charOffset;

    if (properties == 0)
    {
        return false;
    }

    sprintf(sYeelightCommandString, "{\"id\":%d,\"method\":\"get_prop\",\"params\":[%n", sYeelightCommandID, &charOffset);

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

    display_command();

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

