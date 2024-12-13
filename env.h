#pragma once
#include "debug.h"


#define DEFAULTS_ENV
#define HEADER_STRING   "Ворота кооператива \"Озеро\""
#define BUTTON_NAME     "Открыть"
#define OPEN_REPORT     "Ворота открыты, мой господин"
#define DEFAULT_TZ      "UTC0" //"MSK-3"
#define DEFAULT_TZ_MSK  "MSK-3"

#define _SAY_HI_MD_  "_Привет\\. Я снова тут\\.\\.\\._"
#define NTP_SERVERS "ntp1.stratum2.ru" , "ru.pool.ntp.org", "pool.ntp.org"
#define _TRY_LATTER_ "Что-то пошло не так. Попробуйте ещё раз."
#define _CHANNEL_FOR_CONTROL_ "Мой канал управления "
static const char * CHANNEL_FOR_CONTROL PROGMEM = _CHANNEL_FOR_CONTROL_;
static const char * SAY_HI_MD PROGMEM = _SAY_HI_MD_;
static const char * TRY_LATTER PROGMEM = _TRY_LATTER_;

namespace Version {
    static const char * app PROGMEM = "TelegramOpener";
    struct Version {
        const uint8_t high = 0;
        const uint8_t middle = 0;
        const uint8_t low = 0;
        const char * betta = nullptr;
    };
    
    bool isBetta(Version& version){
        return  version.betta != nullptr;
    };
    String getString(Version& version){
        String v;
        v += version.high;
        v += '.';
        v += version.middle;
        v += '.';
        v += version.low;
        if ( isBetta(version) ){
            v += version.betta;
        }
        return v;
    };
    String getApp(){
        return String(app);
    };
    String appVersion(Version& version){
        String s = getApp();
        s += F(" v");
        s += getString(version);
        return s;
    }
}