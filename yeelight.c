
#include "yeelight.h"

#include "uart.h"

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

#define UART_COMMAND_BUFFER (80)

static char sUARTCommandBuffer[UART_COMMAND_BUFFER];
static int sUARTCurrentCharacter = 0;

/*
Simple protocol

<light number> <c><i/s> <value> - colour temp (1700-6500)
<light number> <r><i/s> <value> - rgb (rrggbb, hex digits)
<light number> <h><i/s> <value> <value> - hsv (0-359 0-100)
<light number> <b><i/s> <value> - brightness (1-100)
<light number> <p><i/s> <value> - power (0,1)
<light number> <t><i/s> - toggle
<light number> <d> - default
*/

static const char const sValidCommands[]= "^crhbptd";

#define SIMPLE_COMMAND_EFFECT_INSTANT   ('i')
#define SIMPLE_COMMAND_EFFECT_SMOOTH   ('s')

typedef enum YeelightSimpleCommandParsePhaseEnum
{
    kPhaseLight,
    kPhaseCommand,
    kPhaseEffect,
    kPhaseValueInt1,
    kPhaseValueHex1,
    kPhaseValueInt2,
    kPhaseComplete,

} YeelightSimpleCommandParsePhase;

static YeelightSimpleCommandParsePhase sInitialCommandRead[] = { kPhaseLight, kPhaseCommand };

static YeelightSimpleCommandParsePhase sCommandColourTemperature[] = { kPhaseEffect, kPhaseValueInt1, kPhaseComplete };
static YeelightSimpleCommandParsePhase sCommandRGB[] = { kPhaseEffect, kPhaseValueHex1, kPhaseComplete };
static YeelightSimpleCommandParsePhase sCommandHSV[] = { kPhaseEffect, kPhaseValueInt1, kPhaseValueInt2, kPhaseComplete };
static YeelightSimpleCommandParsePhase sCommandBrightness[] = { kPhaseEffect, kPhaseValueInt1, kPhaseComplete };
static YeelightSimpleCommandParsePhase sCommandPower[] = { kPhaseEffect, kPhaseValueInt1, kPhaseComplete };
static YeelightSimpleCommandParsePhase sCommandToggle[] = { kPhaseEffect, kPhaseComplete };
static YeelightSimpleCommandParsePhase sCommandDefault[] = { kPhaseComplete };

static YeelightSimpleCommandParsePhase *sSpecificCommandRead[] =
{
    NULL,
    sCommandColourTemperature,
    sCommandRGB,
    sCommandHSV,
    sCommandBrightness,
    sCommandPower,
    sCommandToggle,
    sCommandDefault,
};

static int sCommandFailure;

typedef enum YeelightSimpleCommandFailureEnum
{
    kFailureOK,
    kFailureLight,
    kFailureCommand1,
    kFailureCommand2,
    kFailureEffect,
    kFailureInt,
    kFailureHex,
} YeelightSimpleCommandFailure;

bool process_received_command(const char *commandBuffer)
{
    YeelightSimpleCommandParsePhase *phaseControl = sInitialCommandRead;
    int currentPhase;

    int light = 0;
    YeelightCommandID command = 0;
    int value1 = 0;
    int value2 = 0;
    YeelightEffectType effect = kTLEffectSudden;

    size_t commandLength = strlen(sUARTCommandBuffer);
    sCommandFailure = 0;

    int i,j;
    for(i=0;i<commandLength;i++)
    {
        int commandChar = sUARTCommandBuffer[i];

        if (isspace(commandChar) || commandChar == 0)
        {
            currentPhase++;
        }

        switch(phaseControl[currentPhase])
        {
            case kPhaseLight:
                if (isdigit(commandChar))
                {
                    light *= 10;
                    light += commandChar - '0';
                }
                else
                {
                    return false;
                }
                break;
            case kPhaseCommand:

                if (!isalpha(commandChar))
                {
                    sCommandFailure = kFailureCommand1;
                    return false;
                }

                for(j=1;j<sizeof(sValidCommands)/sizeof(char);j++)
                {
                    if (sValidCommands[j] == commandChar)
                    {
                        command = kYLSupportGetProp + j; // SO BAD!
                    }
                }

                if (command == 0)
                {
                    sCommandFailure = kFailureCommand2;
                    return false;
                }

                phaseControl = sSpecificCommandRead[command];
                currentPhase = 0;
                break;
            case kPhaseEffect:
                if (commandChar == SIMPLE_COMMAND_EFFECT_SMOOTH)
                {
                    effect = kTLEffectSmooth;
                }
                else
                if (commandChar != SIMPLE_COMMAND_EFFECT_INSTANT)
                {
                    sCommandFailure = kFailureEffect;
                    return false;
                }
                break;
            case kPhaseValueInt1:
                break;
            case kPhaseValueInt2:
                break;
            case kPhaseValueHex1:
                break;
            case kPhaseComplete:
                break;
        }
    }

    return true;
}

void uart_add_to_buffer(uint8 uart, uint8 transmittedCharacter)
{
    if (sUARTCurrentCharacter >= UART_COMMAND_BUFFER)
    {
        return;
    }

    if (transmittedCharacter == 0xd)
    {
        return;
    }

    if (transmittedCharacter == 0xa)
    {
        sUARTCommandBuffer[sUARTCurrentCharacter] = 0;
        sUARTCurrentCharacter = 0;

        process_received_command(sUARTCommandBuffer);

        return;
    }

    sUARTCommandBuffer[sUARTCurrentCharacter] = transmittedCharacter;
    sUARTCurrentCharacter++;
}

static void uart0_rx_intr_handler(void *parameter)
{
    /* uart0 and uart1 intr combine togther, when interrupt occur, see reg 0x3ff20020, bit2, bit0 represents
    * uart1 and uart0 respectively
    */
    uint8 RcvChar;
    uint8 uart_no = UART0;
    uint8 fifo_len = 0;

    uint32 uart_intr_status = READ_PERI_REG(UART_INT_ST(uart_no)) ;

    while (uart_intr_status != 0x0)
    {
        if (UART_FRM_ERR_INT_ST == (uart_intr_status & UART_FRM_ERR_INT_ST))
        {
            WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_FRM_ERR_INT_CLR);
        }
        else if (UART_RXFIFO_FULL_INT_ST == (uart_intr_status & UART_RXFIFO_FULL_INT_ST))
        {
            fifo_len = (READ_PERI_REG(UART_STATUS(UART0)) >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT;

            for(;fifo_len;fifo_len--)
            {
                uart_add_to_buffer(UART0, READ_PERI_REG(UART_FIFO(UART0)) & 0xFF);
            }

            WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
        }
        else if (UART_RXFIFO_TOUT_INT_ST == (uart_intr_status & UART_RXFIFO_TOUT_INT_ST))
        {
            fifo_len = (READ_PERI_REG(UART_STATUS(UART0)) >> UART_RXFIFO_CNT_S) & UART_RXFIFO_CNT;

            for(;fifo_len;fifo_len--)
            {
                uart_add_to_buffer(UART0, READ_PERI_REG(UART_FIFO(UART0)) & 0xFF);
            }

            WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);
        }
        else if (UART_TXFIFO_EMPTY_INT_ST == (uart_intr_status & UART_TXFIFO_EMPTY_INT_ST))
        {
            WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_TXFIFO_EMPTY_INT_CLR);
            CLEAR_PERI_REG_MASK(UART_INT_ENA(UART0), UART_TXFIFO_EMPTY_INT_ENA);
        }

        uart_intr_status = READ_PERI_REG(UART_INT_ST(uart_no));
    }
}

void yeelight_start()
{
    xTaskCreate(task_udp, sYeeLightTask, MAX_STACK_SIZE, NULL, TASK_PRIORITY, &yeelightConnection.mTaskUDPHandle);

    _xt_isr_attach(ETS_UART_INUM, uart0_rx_intr_handler, NULL);
    ETS_UART_INTR_ENABLE();

    os_printf("Finally here\n");
}

