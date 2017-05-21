#include "yeelight.h"

YeelightData *list_search_light(int32 highID, int32 lowID)
{
    YeelightData *light;

    for(light = yeelightsList; light; light = light->mNext)
    {
        if (light->mIDHigh == highID && light->mIDLow == lowID)
        {
            return light;
        }
    }

    return NULL;
}

YeelightData *list_search_light_local(int32 id)
{
    YeelightData *light;

    for(light = yeelightsList; light; light = light->mNext)
    {
        if (light->mLocalID == id)
        {
            return light;
        }
    }

    return NULL;
}


void list_add(YeelightData *light)
{
    light->mNext = yeelightsList;
    yeelightsList = light;
}

int list_count()
{
    YeelightData *light;
    int count = 0;

    for(light = yeelightsList; light; light = light->mNext)
    {
        count++;
    }

    return count;
}

void list_lights()
{
    YeelightData *light;

    os_printf("Lights: %d\n", list_count());

    for(light = yeelightsList; light; light = light->mNext)
    {
        os_printf("Light: %08x%x08:%d, power: %d\n", light->mIDHigh, light->mIDLow, light->mLocalID, light->mPower);
    }
}
