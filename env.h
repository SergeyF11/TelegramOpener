#pragma once
#include "debug.h"


#define DEFAULTS_ENV
#define HEADER_STRING   "Ворота кооператива \"Озеро\""
#define BUTTON_NAME     "Открыть"
#define OPEN_REPORT     "Ворота открыты, мой господин"
#define DEFAULT_TZ      "UTC0" //"MSK-3"
#define DEFAULT_TZ_MSK  "MSK-3"

#define _SAY_HI_MD_  "_Привет\\. Я снова тут\\.\\.\\._"
static const char SAY_HI_MD[] PROGMEM = _SAY_HI_MD_;


#define _SAY_HI_  "Привет. Я снова тут..."
#define NTP_SERVERS "ntp1.stratum2.ru" , "ru.pool.ntp.org", "pool.ntp.org"
#define _TRY_LATTER_ "Что-то пошло не так. Попробуйте ещё раз."
#define _CHANNEL_FOR_CONTROL_ "Мой канал управления "
#define NULL_STR (char*)nullptr

static const char CHANNEL_FOR_CONTROL[] PROGMEM = _CHANNEL_FOR_CONTROL_;
static const char SAY_HI[] PROGMEM = _SAY_HI_;
static const char TRY_LATTER[] PROGMEM = _TRY_LATTER_;


#include "TelegramMD.h"

class AddedString /* : public String */ {
    private:
        String * s;
        char _delimeter = '\0';
        AddedString _concat(const char * _s){
            if ( _delimeter != '\0' && _s != nullptr && _s[0] != _delimeter ) {
                *s += _delimeter; 
            } 
            *s += _s;
            return *this;    
        };
    public:
    //AddedString(){};
    AddedString(String& _s) : s(&_s) {};
    void setDelimeter(const char d){
        _delimeter = d;
    };

    AddedString operator<<(const char * _s){
        return _concat(_s);
    };
    AddedString operator<<(const String& _s){
        return _concat(_s.c_str());        
    };
    AddedString operator<<(const unsigned int _s){
        return _concat(String(_s).c_str());
    };
    AddedString operator<<(const int _s){
        return _concat(String(_s).c_str());
    };

     operator String() const {
        return *s;
    };

};
namespace TimeRus {
    enum T {
        HOURS,
        MINUTE,
        SECONDS,
    };
    static const char _hours1[] PROGMEM = "час";
    static const char _hours2[] PROGMEM = "часа";
    static const char _hours3[] PROGMEM = "часов";
    static const char _minutes1[] PROGMEM = "минута";
    static const char _minutes2[] PROGMEM = "минуты";
    static const char _minutes3[] PROGMEM = "минут";
    static const char _seconds1[] PROGMEM = "секунда";
    static const char _seconds2[] PROGMEM = "секунды";
    static const char _seconds3[] PROGMEM = "секунд";

    static const char * const _t[3][3] PROGMEM = { 
    {_hours1,_hours2 ,_hours3},
    { _minutes1, _minutes2,_minutes3},
    { _seconds1,_seconds2,_seconds3 }
    };
    enum SUFFIX {
        ONE,
        TWO_FOUR,
        OTHER,
    };
    // static const char * const _t[3][3] PROGMEM = {
    //                 {"час", "часа", "часов"},
    //                 {"минута", "минуты","минут"},
    //                 {"секунда","секунды", "секунд"}};

    auto _timeSuffix = [](T t, const unsigned int s){
        switch(s<20 ? s : s%10){
            case 1:
            return _t[t][SUFFIX::ONE];
            case 2:
            case 3:
            case 4:
            return _t[t][SUFFIX::TWO_FOUR];
        }
        return _t[t][SUFFIX::OTHER];
        };
    void uptimeTo(String& up){
        up = F("Uptime");
        AddedString uptime(up);
        uptime.setDelimeter(' ');
        auto now = millis()/1000;
        auto mins = now/60;
        unsigned int hours = mins/60;
        if ( hours > 0 ){
        uptime << hours << _timeSuffix( TimeRus::HOURS, hours );
        mins = mins - hours*60;
        }
        if ( mins > 0 ){
        uptime << (unsigned int)mins << _timeSuffix( TimeRus::MINUTE, mins);
        }
        unsigned secs = now%60;
        if ( secs > 0 || ( hours == 0 && mins == 0 )) {
        uptime << secs << _timeSuffix( TimeRus::SECONDS, secs);
        }
    };
    String uptime(){
        String out;
        uptimeTo(out);
        return out;
    };
    
};

namespace Url {
    //String& slah(const char * s){};
    String& slash(String& s, const char * add){
        if( add != nullptr && add[0] != '/' ){
            s += '/';
        }
        s += add;
        return s;
    };
    // String _preSlash(const char * s){
    //     String out;
    //     if( s != nullptr && s[0] != '/' ){
    //         out += '/';
    //     }
    //     out += s;
    //     return out;
    // };
};

namespace Author {
    static const char firstName[] PROGMEM = "Sergey";
    static const char secondName[] PROGMEM = "Fedotov";
    static const char gitHubAka[] PROGMEM = "SergeyF11";
       
    String getName(){
        String name(firstName);
        name += ' ';
        name += secondName;
        return name;
    };
    String getCopyright(){
        static const char _copyright[] PROGMEM = "Copyright (C) ";
        String c = ( _copyright);
        c += getName();
        c += F(" aka ");
        c += gitHubAka;
        return c;
    }
};
namespace App {
    static const char name[] PROGMEM = "TelegramOpener";
//    static const char dataCerts[] PROGMEM = "data/certs.ar";
    static const char GITHUB_IO_FINGERPRINT[] PROGMEM = "97:D8:C5:70:0F:12:24:6C:88:BC:FA:06:7E:8C:A7:4D:A8:62:67:28";
    const int gitHubPort = 443;

    struct Version {
        //private:
        uint8_t high = 0;
        uint8_t middle = 0;
        uint8_t low = 0;
        char * betta = nullptr;
        void addBetta(const char * b){
            debugPretty;
            if ( b != nullptr && *b != '\0' ){
                betta = (char *)malloc( strlen(b));
                strcpy(betta,b);
            }
        }
        //public:
        // Version(){};
        // Version(uint8_t h, uint8_t m, uint8_t l, const char* b=nullptr) :
        //     high(h), middle(m), low(l), betta(b)
        // {};
        Version(){};
        Version(const uint8_t h,const uint8_t m,const uint8_t l, const char* b=nullptr){
            high =h;
            middle = m;
            low =l;
            addBetta(b);
        };
        Version(const String& s){
            fromString(s);
        };
        ~Version(){
            if ( betta != nullptr ) {
                free(betta);
            }
        };
        bool operator==(const Version& a) const {
            return ( 
                high == a.high && 
                middle == a.middle && 
                low == a.low && 
                (isBetta() == a.isBetta())
            );
        };
        
        bool operator>(const Version& a) const {
            bool res = false;
            if ( high > a.high ) res = !res; //return true;
            else if ( high == a.high ) {
                    if( middle > a.middle ) res =!res; //return true;
                    else if ( middle == a.middle ){
                        if( low > a.low ) res =!res; //return true;
                        else if( low == a.low ){
                            if( (! isBetta() ) && a.isBetta() ) res =!res; //return true;
                        }
                    }
            }
            return res; //false; 
        };
        bool operator<(const Version& a) const { return a > (*this); };
        bool operator<=(const Version& a) const { return ! ( (*this) > a ); };
        bool operator>=(const Version& a) const { return ! ( a > (*this) ); };
        bool operator!=(const Version& a) const { return ! ( (*this == a)); };
        bool isBetta() const {
            return betta != nullptr;
        };
        String toString() const {
            String v(high);
            v += '.';
            v += middle;
            v += '.';
            v += low;
            if ( isBetta() ){
                v += betta;
            }
            return v;
        };

        void fromString(const String& s ) {
            // debugPretty;
            // debugPrintln(s);
            if( s.isEmpty() ){
                debugPretty;
                // high = 0;
                // middle = 0;
                // low = 0;
                return;
            }
            char* buf = (char *)malloc(s.length()+1 );
            strcpy( buf, s.c_str());

            char * v = strtok( buf, ".");
            high = atoi(v); 
            v = strtok( NULL, ".");
            middle = atoi(v);

            v = strtok( NULL, ".");

            if ( v != NULL ) {
                low = atoi(v);
                int bettaP = String(low).length();
                if ( bettaP < strlen(v) ) 
                    addBetta( (const char *)(v + bettaP) );
            } else low = 0;
            free(buf);
        };            
    };


    String getBinFile(){
        String bin(name);
        bin += F(".ino.bin");
        return bin;
    };
    // bool isBetta(Version& version){
    //     return  version.betta != nullptr;
    // };
    // String getString(Version& version){
    //     String v;
    //     v += version.high;
    //     v += '.';
    //     v += version.middle;

    //     if ( isBetta(version) ){
    //         v += '.';
    //         v += version.low;
    //         v += version.betta;
    //     }
    //     return v;
    // };
    String getApp(){
        return String(name);
    };
    String appVersion(Version& version, const char * data = nullptr, const char * time = nullptr ){
        String s = getApp();
        s += F(" v");
        s += version.toString(); //getString(version);
        for( auto next : {data,time} ){
            if ( next != nullptr){
                s += ' ';
                s += next;
            }
        }
        return s;
    }
    String getHomePage(){
        String out(F("https://github.com/"));
        out += Author::gitHubAka;
        //out += '/';
        /* out += */ Url::slash(out, App::name );
        return out;
    };
    static const char gitHubUserContent[] PROGMEM = "https://raw.githubusercontent.com";
    static const char * gitHubUserContentHost = gitHubUserContent + 8;

    String getRawContent(const char * fileName, bool host=true){
        String out;
        if(  host ) out += gitHubUserContent;
//        out += '/';
        Url::slash( out, Author::gitHubAka);
        //out += '/';
        Url::slash( out, App::name );
        Url::slash( out, PSTR("refs/heads/main"));
        //out += F("/refs/heads/main");
        Url::slash( out,fileName );
        return out;
        //SergeyF11/TelegramOpener/refs/heads/main/README_rus.pdf"))
    }
}
namespace Time {
    bool isSynced(){
        return time(nullptr) > 3600*2*6;
    };
    static char * buf = nullptr;
    void _free(){
            if ( buf != nullptr ) { 
                free(buf);
                buf = nullptr;
            }
        };
    char * toStr(const time_t& now = time(nullptr))  {
        static constexpr char tmpl[] PROGMEM = "%4d-%02d-%02d %02d:%02d:%02d";
        //auto now = time(nullptr);
        auto _tm = localtime( &now );
        _free();
        buf = (char *)malloc(20);
        sprintf(buf, tmpl, 
            _tm->tm_year+1900, _tm->tm_mon+1, _tm->tm_mday, 
            _tm->tm_hour, _tm->tm_min, _tm->tm_sec );
        return buf;
    }
    size_t printTo(Print& p){
        auto size =  p.print(toStr());
        _free();
        return size;
    };

}