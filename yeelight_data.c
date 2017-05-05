#include "yeelight.h"

const char const sYeeLightSearchCommand[]    = "M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1982\r\nMAN: \"ssdp:discover\"\r\nST: wifi_bulb\r\n";
const char const sYeeLightMulicastAddress[]  = "239.255.255.250";

int yeelight_search_command_size()
{
    return sizeof(sYeeLightSearchCommand) - 1;
}

const char * const sYeeLightCommandList[] =
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

const char * const sYeelightProcessSearches[] =
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

const char * const sYeeLightModelList[] =
{
    "mono",
    "color",
    "stripe",
};

const char *const sYeelightProperties[] =
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

int sYeelightID = 1;

char sYeelightCommandString[kMaxCommandString];// = "{\"id\":1,\"method\":\"get_prop\",\"params\":[\"id\",\"power\",\"bright\"]}\r\n";

YeelightConnectionData yeelightConnection;
