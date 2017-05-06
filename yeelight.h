#ifndef __YEELIGHT_H__
#define __YEELIGHT_H__

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

#define YEE_DEBUG

#ifndef YEE_DEBUG
#undef os_printf
#define os_printf
#endif

#define kSuppportCommandSize    (16)

#define copyStringReturnEnd     (0)
#define copyStringInvalidChar   (-1)
#define copyStringCmdTooLong    (-2)

#define kMaxCommandString       (256)

typedef struct YeelightConnectionDataStruct
{
    struct espconn	mESPConnection;
    esp_udp			mESPUDP;
    struct espconn	mESPConnectionTCP;
    esp_tcp			mESPTCP;
    xTaskHandle     mTaskUDPHandle;

    const char      *searchStart;
    const char      *searchEnd;

    bool            mIGMPJoined;
    bool            mBulbQueryComplete;

} YeelightConnectionData;

typedef enum YeeLightModelEnum
{
    YLModelMono,
    YLModelColour,
    YLModelStripe,

    YLModelCount,
} YeeLightModel;

typedef enum YeeLightColourModeEnum
{
    YLColourModeColour,
    YLColourModeColourTemperature,
    YLColourModeHSV,
} YeeLightColourMode;

// Compiler is not convinced that this is constant
//const int kMaxYLName             = 64;
#define kMaxYLName             (64)

typedef struct YeelightIDStruct
{
    int32               mIDHigh;
    int32               mIDLow;
} YeelightID;

typedef struct YeelightDataStruct
{
    int32               mIDHigh; // These must be at the top and must match the YeelightID struct
    int32               mIDLow;

    char                mAssignedName[kMaxYLName];
    ip_addr_t           mIPAddress;
    uint16              mPort;
    int32               mSupports;
    int32               mFirmwareVersion;

    int32               mRGB;
    YeeLightModel       mModel;
    sint16              mColourTemperature;
    sint16              mHue;
    u8                  mColourMode;
    u8                  mSaturation;
    u8                  mBrightness;
    u8                  mLocalID;
    bool                mPower;

    struct YeelightDataStruct   *mNext;
} YeelightData;

typedef enum YeeLightCommandIDEnum
{
    kYLSupportGetProp,
    kYLSupportSetCTAbx,
    kYLSupportSetRGB,
    kYLSupportSetHSV,
    kYLSupportSetBright,
    kYLSupportSetPower,
    kYLSupportToggle,
    kYLSupportSetDefault,
    kYLSupportStartCF,
    kYLSupportStopCF,
    kYLSupportSetScene,
    kYLSupportCronAdd,
    kYLSupportCronGet,
    kYLSupportCronDel,
    kYLSupportSetAdjust,
    kYLSupportSetMusic,
    kYLSupportSetName,

    kYLSupportCount,
} YeeLightCommandID;

enum YeeLightProcessSearchID
{
    kYLProcessSearchID,
    kYLProcessSearchYeelight,
    kYLProcessSearchModel,
    kYLProcessSearchFirmware,
    kYLProcessSearchSupport,
    kYLProcessSearchPower,
    kYLProcessSearchBrightness,
    kYLProcessSearchColourMode,
    kYLProcessSearchColourTemperature,
    kYLProcessSearchRGB,
    kYLProcessSearchHue,
    kYLProcessSearchSat,
    kYLProcessSearchName,

    kYLProcessSearchCount,
};

enum YeelightProperties
{
    kYLPropertyPower = 1 << 0,
    kYLPropertyBrightness = 1 << 1,
    kYLPropertyColourTemperature = 1 << 2,
    kYLPropertyRGB = 1 << 3,
    kYLPropertyHue = 1 << 4,
    kYLPropertySat = 1 << 5,
    kYLPropertyColourMode = 1 << 6,
    kYLPropertyFlowing = 1 << 7,
    kYLPropertyDelayOff = 1 << 8,
    kYLPropertyFlowParams = 1 << 9,
    kYLPropertyMusicOn = 1 << 10,
    kYLPropertyName = 1 << 11,

    kYLPropertyCount = 12,
};

extern void yeelight_start();

// yeelight_data
extern YeelightConnectionData yeelightConnection;
extern YeelightData *yeelightsList;
extern int sYeelightCommandID;
extern int sYeelightNextID;

extern char sYeelightCommandString[kMaxCommandString];

extern const char const sYeeLightSearchCommand[];
extern const char const sYeeLightMulicastAddress[];
extern const char * const sYeeLightCommandList[];
extern const char * const sYeelightProcessSearches[];
extern const char * const sYeeLightModelList[];
extern const char *const sYeelightProperties[];
extern bool (*sYeelightProcessSearchFunctions[])(const char *dataPointer, YeelightData *yeelightData);
extern int yeelight_search_command_size();

// yeelight_udp
extern void task_udp(void *pvParameters);

//yeelight_parse
extern const char *search_packet(const char * const searchTerm);
extern void search_packet_init(const char *searchPosition, unsigned short searchLength);

// yeelight.c
extern bool command_get_prop(YeelightConnectionData *yeelightData, YeelightData *bulb, int properties);

// yeelight_tcp
extern bool tcp_send_command(YeelightConnectionData *yeelightData, YeelightData *bulb);

// yeeligh_list
extern YeelightData *list_search_light(int32 highID, int32 lowID);
extern void list_add(YeelightData *light);
extern int list_count();
extern void list_lights();


#endif
