#include "yeelight.h"

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

bool (*sYeelightProcessSearchFunctions[])(const char *dataPointer, YeelightData *yeelightData) =
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
