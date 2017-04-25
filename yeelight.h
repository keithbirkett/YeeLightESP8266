#ifndef __YEELIGHT_H__
#define __YEELIGHT_H__

extern void yeelight_start();

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

typedef struct YeelightDataStruct
{
    char                mAssignedName[kMaxYLName];
    ip_addr_t           mIPAddress;
    uint16              mPort;
    int32               mIDHigh;
    int32               mIDLow;
    int32               mSupports;
    int32               mFirmwareVersion;

    int32               mRGB;
    YeeLightModel       mModel;
    sint16              mColourTemperature;
    sint16              mHue;
    u8                  mColourMode;
    u8                  mSaturation;
    u8                  mBrightness;
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

extern bool command_get_prop(YeelightConnectionData *yeelightData, YeelightData *bulb, int properties);

extern bool tcp_send_command(YeelightConnectionData *yeelightData, YeelightData *bulb);

#endif
