#pragma once
#include "debug.h"


#define DEFAULTS_ENV
#define HEADER_STRING   "Ворота кооператива \"Озеро\""
#define BUTTON_NAME     "Открыть"
#define OPEN_REPORT     "Ворота открыты, мой господин"
#define DEFAULT_TZ      "UTC0" //"MSK-3"
#define DEFAULT_TZ_MSK  "MSK-3"

#define _SAY_HI_MD_  "_Привет\\. Я снова тут\\.\\.\\._"
static const char * SAY_HI_MD PROGMEM = _SAY_HI_MD_;


#define _SAY_HI_  "Привет. Я снова тут..."
#define NTP_SERVERS "ntp1.stratum2.ru" , "ru.pool.ntp.org", "pool.ntp.org"
#define _TRY_LATTER_ "Что-то пошло не так. Попробуйте ещё раз."
#define _CHANNEL_FOR_CONTROL_ "Мой канал управления "
static const char * CHANNEL_FOR_CONTROL PROGMEM = _CHANNEL_FOR_CONTROL_;
static const char * SAY_HI PROGMEM = _SAY_HI_;
static const char * TRY_LATTER PROGMEM = _TRY_LATTER_;

#include "TelegramMD.h"
// namespace MARKDOWN_TG{
//     static const char chars[] PROGMEM = "_*[]()~`>#+-=|{}.!";
//     String escape(const char * txt){
//         debugPretty;
//         String dest;
//         dest.reserve( 2*strlen( txt));
//         int j=0;
//         //char srs = txt[j];
//         while ( txt[j] != '\0'){
//             debugPrint( txt[j] );
//             bool escaped = false;
//             int i=0;
//             //char c = chars[i];
//             while ( chars[i] != '\0' ){
//                 if ( txt[j] == chars[i] ) {
//                     escaped = true;
//                     break;
//                 }
//                 i++;
//             }
//             if ( escaped ) {
//                 dest += '\\';
//             }
//             dest += txt[j];
//             debugPrint("=>");
//             debugPrintln( dest );
//             j++;
//         }
//         return dest;
//     };
// }

namespace Author {
    static const char * firstName PROGMEM = "Sergey";
    static const char * secondName PROGMEM = "Fedotov";
    static const char * gitHubAka PROGMEM = "SergeyF11";
    String getName(){
        String name(firstName);
        name += ' ';
        name += secondName;
        return name;
    };
    String getCopyright(){
        static const char * _copyright PROGMEM = "Copyright (C) ";
        String c = ( _copyright);
        c += getName();
        c += F(" aka ");
        c += gitHubAka;
        return c;
    }
};
namespace App {
    static const char * name PROGMEM = "TelegramOpener";
    struct Version {
        //private:
        uint8_t high = 0;
        uint8_t middle = 0;
        uint8_t low = 0;
        char * betta = nullptr;
        void addBetta(const char * b){
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
        bool operator<(const Version& a) const { return a>*this; };

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
            debugPretty;
            debugPrintln(s);

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
            }
            else low = 0;
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
        out += '/';
        out += App::name;
        return out;
    };
    String getRawContent(const char * suffix){
        String out( F("https://raw.githubusercontent.com/"));
        out += Author::gitHubAka;
        out += '/';
        out += App::name;
        out += suffix;
        return out;
        //SergeyF11/TelegramOpener/refs/heads/main/README_rus.pdf"))
    }
}