#ifndef __YEELIGHT_H__
#define __YEELIGHT_H__

extern void yeelight_ip_callback();

#define kSuppportCommandSize    (16)

#define copyStringReturnEnd     (0)
#define copyStringInvalidChar   (-1)
#define copyStringCmdTooLong    (-2)

typedef struct YeelightConnectionDataStruct
{
    struct espconn	mESPConnection;
    esp_udp			mESPUDP;
    xTaskHandle     mTaskUDPHandle;

    const char      *searchStart;
    const char      *searchEnd;

    bool            mIGMPJoined;

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

#endif
