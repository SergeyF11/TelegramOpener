#pragma once

//#include <ESP_OTA_GitHub.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
//#include "downloadCerts.h"
#include "env.h"
#include <time.h>
#include "myFastBotClient.h"
#include "newFsSettings.h"
//#include "myFileDb.h"
#include "myPairs.h"
#include <CertStoreBearSSL.h>


extern BotSettings::Settings settings;
extern CertStore * certStore;
extern FastBot2Client bot;
extern App::Version version;
extern MenuIds menuIds;
extern WiFiClientSecure client;


namespace GitHubUpgrade {
    static const char apiHost[] PROGMEM = "api.github.com";
    static const char latest[] PROGMEM = "/releases/latest";
    static const int port = 443;
    static const char contentType[] PROGMEM = "application/octet-stream";


    enum Errors {
        Ok,
        No_New_Version,
        Failed_Connection,
        Failed_Host_Response,
        No_Valid_Binary,
        No_Tag_Name,
        Failed_JSON_Parse,
        PreRelease_Version,
    };
    static Errors _lastErrorCode;
    static struct Release {
        char tag[9] = "0.0.0dbg";
        enum Url {
            Download,
            Info,
        };
        char * _downloadUrl = nullptr;
        char * _infoUrl = nullptr;
        void clean(){
            if ( _downloadUrl != nullptr ){
                free(_downloadUrl);
                _downloadUrl = nullptr;
            }
            if ( _infoUrl != nullptr  ){
                free(_infoUrl);
                _infoUrl = nullptr; 
            }
        };
        //bool release.has = false;
        String constructUrl( Url typeUrl){
            String out( App::getHomePage() );
            out += latest;
            out = out.substring(0, out.length()-6);
            if ( typeUrl == Url::Download ){
                out += F("download/");
                out += tag;
                out += '/';
                out += App::getBinFile();
            } else {
                out += F("tag/");
                out += tag;
            }
            debugPretty;
            debugPrintln(out);
            return out;
        };
        bool canConstruct( const char * url, Url typeUrl ){
            String constructed( constructUrl( typeUrl));
            return constructed.equals( url ); 
        };
        bool constructed[2] = {false};
        String getUrl(Url typeUrl){
            if ( constructed[typeUrl] ) return constructUrl(typeUrl);
            // else
            return String( typeUrl == Url::Download ? _downloadUrl : _infoUrl );
        };
        bool has = false;
    } release;
    static bool needUpgrade = false;
    // static bool has = false;
    // char _releaseTag[] = "0.0.0dbg";
    // //static const String appBinFile = App::getBinFile();
    

    // //static const time_t _likeRealTime =  2*24*60*60;
    // 
    // // static int checkedDay=0;
    // //static String latestTag;
    // static char * _downloadUrlPtr = nullptr;
    // static char * _InfoUrlPtr = nullptr;

    // // static String _downloadURL = NULL_STR;
    // // static String _InfoURL = NULL_STR;
    
    // void stringClean(){
    //     free(_downloadUrlPtr);
    //     _downloadUrlPtr = nullptr;
    //     free(_InfoUrlPtr);
    //     _InfoUrlPtr = nullptr;

    //     // _downloadURL= NULL_STR;
    //     // _InfoURL = NULL_STR;
    //     // _downloadURL.reserve(0);
    //     // _InfoURL.reserve(0);
    // };
    static const char _sun[] PROGMEM ="Sun";
    static const char _mon[] PROGMEM ="Mon";
    static const char _tue[] PROGMEM ="Tue";
    static const char _wed[] PROGMEM ="Wed";
    static const char _thu[] PROGMEM ="Thu";
    static const char _fri[] PROGMEM ="Fri";
    static const char _sat[] PROGMEM ="Sat";
    static const char _anyStr[] PROGMEM ="Any";
    static const char * const _weekDays[] PROGMEM = { _sun, _mon, _tue, _wed, _thu,_fri, _sat};
    const char * weekDayStr(uint day){ return _weekDays[day]; };
        //"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    struct At /*  : public Printable  */{
        enum WeekDays {
            Any = -1,
            Sun,
            Mon,
            Tue,
            Wed,
            Thu,
            Fri,
            Sat,
        };
        
        int weekDay=5;
        int hour=4;
        int min=0;
        int _checkedDay=WeekDays::Any;
        bool setCheckedDay(int cd){
            return _checkedDay = cd;
        }; 
        // bool setCheckedDay(int cd){ 
        //     bool valid =( cd > WeekDays::Any && cd <= WeekDays::Sat );
        //     if ( valid ) _checkedDay=cd;
        //     return valid;
        // };
        bool setDay(const char * cd){
            bool valid = false;
            for( int i=WeekDays::Sun; i<= WeekDays::Sat; i++ ){
                if( strncmp( cd, _weekDays[i], 3) == 0 ){
                    weekDay = i;
                    valid = !valid;
                }
            }
            return valid;
        };
        bool isAny(const int val){
            return val == WeekDays::Any;
        };
        bool addToString(String& s, const int val, const char * (*getStr)(uint)=nullptr){
            bool res = isAny(val); 
            if ( res ) s += _anyStr;
            else if ( getStr == nullptr ) {
                if ( val < 10 ) s += '0';
                s += val;
            } else {
                s += getStr(val);
            }
            return res;
        };
        String toString(){
            String out(F("At: "));
            addToString( out, weekDay, weekDayStr);
            out += ' ';
            addToString(out, hour);
            out += ':';
            addToString(out, min);
            return out;
        };
        size_t printTo( Print& s){
            return s.print(toString());
        };
        bool checkedDay(const time_t* setDay=nullptr){     
            if ( ! Time::isSynced() ) {
                debugPrintln("No sync time");
                return true;
            }
            if ( setDay != nullptr){
                auto setTime = localtime(setDay);
                _checkedDay = setTime->tm_yday;
                debugPrintf("Set checked day %d\n", _checkedDay);
            } 

            const time_t now = time(nullptr);
            const tm* nowTime =localtime(&now);

            bool res = _checkedDay == nowTime->tm_yday;
            #ifdef debug_print
            if ( res ) {
                static bool printed = false;
                if ( ! printed ) {
                    printed = true;
                    debugPrintln("Checked today already");
                }
            } else {
                debugPrintln("Need check now");
            }
            #endif
            return res;
        };        
        At(const int _weekDay=5, const int _hour=4, const int _min=0 ) :
            weekDay(_weekDay), hour(_hour), min(_min)
            {};
        At* set( const int _weekDay=5, const int _hour=4, const int _min=0 ) /*:
            weekDay(weekDay), hour(hour), min(min) */ {
        //at = At{weekDay, hour, min};
            weekDay = _weekDay;
            hour = _hour;
            min = _min;
            return this;
        };
        //static const int Any = -1;
        static uint8_t Random(uint8_t max){ 
            return ( *(volatile uint8_t *)0x3ff20e44)%max; };
        bool isTime(  ) {
            const time_t now = time(nullptr);
            auto nowTime = localtime(&now);
            //if ( checkedDay(nowTime) ) return false;
            //if ( now == 0 ) now = time(nullptr);
            
            return ( weekDay == Any ||  nowTime->tm_wday == weekDay ) &&
                ( hour == Any || nowTime->tm_hour == hour ) && 
                ( min == Any || nowTime->tm_min == min );
        };
    };
    static At at; 
    
                   
    // void checkAt(const int weekDay=5, const int hour=4, const int min=0 ){
    //     at.set(weekDay, hour, min); 
    // };

    Errors getGitHubRelease(){
        String url( F("/repos")); 
        Url::slash( url, Author::gitHubAka);
        Url::slash( url, App::name );
        Url::slash( url, latest );
        HTTPClient http;
        if ( ! http.begin(client, apiHost, port, url, /*https=*/true )) {
            _lastErrorCode = Errors::Failed_Connection;
        } else {  
            int httpCode = http.GET();
            debugPretty;
            debugPrintf("Get %s:%d %s\n\tResult: %d\n", apiHost, port, url.c_str(), httpCode);
            
            if ( httpCode != HTTP_CODE_OK ) { 
                _lastErrorCode = Errors::Failed_Host_Response;    
            } else {   
                // 200 Ok => parsing response
                gson::Parser doc;
                if ( ! doc.parse( http.getString() ) ) {
                    _lastErrorCode = Errors::Failed_JSON_Parse;
                } else {
                    if( ! doc.has("tag_name")){
                        _lastErrorCode = Errors::No_Tag_Name;
                    } else {
                        doc["tag_name"].toStr( (char *)release.tag ); //_releaseTag);  //toString();
                        String release_name = doc["name"].toString();
                        bool prerelease = doc["prerelease"].toBool();
                        if ( prerelease ) {
                            _lastErrorCode = Errors::PreRelease_Version;
                        } else if ( doc.has("assets") && doc["assets"].isArray() ){ //&& doc["assets"].isArray() ){
                            int i = 0;
                            //bool valid_asset = false;
                            //preset error code
                            _lastErrorCode = Errors::No_Valid_Binary; 

                            while(  doc["assets"][i].isObject() ){
                            
                                String asset_type = doc["assets"][i]["content_type"].toString();
                                String asset_name = doc["assets"][i]["name"].toString();

                                    
                                if (strcmp(asset_type.c_str(), contentType) == 0 && 
                                    strcmp(asset_name.c_str(), App::getBinFile().c_str() ) == 0) 
                                {
                                    auto copyUrl = [](char ** dest, const su::Text::Cstr src){ 
                                        debugPretty;
                                        debugPrint( "Dest adr=" ); 
                                        Serial.println( (long long unsigned int)&(*dest), HEX );
                                        debugPrintln( src );

                                        if ( *dest != nullptr ) free( *dest );
                                        //auto buf = src;
                                        *dest = (char *)malloc( strlen( src ) );
                                        strcpy( *dest, src );
                                        debugPrintln( *dest );
                                    };
                                    auto str = doc["assets"][i]["browser_download_url"].c_str();
                                    release.constructed[Release::Download] = release.canConstruct( str, GitHubUpgrade::Release::Download );
                                    if ( ! release.constructed[Release::Download] ){
                                        //copyUrl( &_downloadUrlPtr, doc["assets"][i]["browser_download_url"].c_str());
                                        copyUrl( &release._downloadUrl, str);
                                    }
                                    str = doc["html_url"].c_str();
                                    release.constructed[Release::Info] = release.canConstruct( str, GitHubUpgrade::Release::Info );

                                    if ( ! release.constructed[Release::Info] ){
                                        copyUrl( &release._infoUrl, str );
                                        //copyUrl( &_InfoUrlPtr, doc["html_url"].c_str());
                                    }
                                    // {
                                    //     auto dUrl = doc["assets"][i]["browser_download_url"].c_str();
                                    //     if( _downloadUrlPtr != nullptr ) free(_downloadUrlPtr);
                                    //     _downloadUrlPtr = (char)malloc( strlen( dUrl) );
                                    //     strcpy(_downloadUrlPtr, dUrl);
                                    // }
                                    // {
                                    //     auto riUrl = doc["html_url"].c_str();
                                    //     _InfoUrlPtr = (char)malloc( strlen( riUrl ));
                                    //     strcpy(_InfoURL, riUrl);

                                    // }
                                    // _downloadURL = doc["assets"][i]["browser_download_url"].toString();
                                    // _InfoURL = doc["html_url"].toString();
                                    _lastErrorCode = Errors::Ok;
                                    break;
                                } 
                                i++;
                            }
                            //doc.reset();
                        }
                    }
                }
            }
        }
        return _lastErrorCode;
    };
    bool _check(){
        release.has = false;
        if ( getGitHubRelease() == Errors::Ok ) {
                release.has = true;
                
            } else {
                debugPretty;
                debugPrint("Error:");
                debugPrintln( _lastErrorCode );    
                
            } 
        return release.has;
    };
    bool checkVersion(){
        if ( release.has) {
            App::Version gitHubV( release.tag ); //_releaseTag ); //gitHubUpgrade->getLatestTag());
            debugPrintf("GitHub newest version is %s\n", gitHubV.toString().c_str());

            if ( version >= gitHubV ) { // version <=
                release.has = false;
                debugPrintf("Current version %s is higher or equals the GitHub version %s\n", 
                    version.toString().c_str(), 
                    gitHubV.toString().c_str());

            } else if ( menuIds.hasIgnoreVersion() ){
                App::Version ignoreVersion(menuIds.getIgnoreVersion());
                if ( ignoreVersion >= gitHubV  ){ //>=
                    release.has = false;
                    debugPrintf("Ignore version up to %s\n", 
                        ignoreVersion.toString().c_str());    
                }
            }
        }
        return release.has;
    };

    bool check(bool now = false){
        if ( ! now  && ( at.checkedDay() || ! at.isTime() ) ) return false;
        // }
        // if( ! now &&/* at.checkedDay() || */ ! at.isTime() ) return false;
        // //if ( at.isTime() ) {   
        // if ( ! now && at.checkedDay() ) {
        //     return false;
        // } 
        else {    
            if ( _check() ) {
                auto now = time( nullptr);
                at.checkedDay( &now );
            };

            
            if ( checkVersion() ) {
                // latestTag = GitHubUpgrade.getLatestTag();
                debugPretty; 
                debugPrintln( release.tag ); //_releaseTag ); //gitHubUpgrade->getLatestTag() );
                // downloadUrl =  GitHubUpgrade.getUpgradeURL();
                
            } else if ( _lastErrorCode != Errors::Ok ) {    
                debugPretty;
                // error = GitHubUpgrade.getLastError();
                debugPrint("Error:" );// gitHubUpgrade->getLastError());
                debugPrintln( _lastErrorCode );
            }
        }

        return release.has;
    };

    String tag(){
        if ( release.has ) return String( release.tag ); //_releaseTag ); //gitHubUpgrade->getLatestTag(); //latestTag;
        return NULL_STR;
    };

    bool doIt(){
        if ( release.has ){
            
            ESPhttpUpdate.setClientTimeout(8000);
            ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
            ESPhttpUpdate.rebootOnUpdate(false);
            ESPhttpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

            debugPretty;
            //debugPrintf("Update %s\n", _downloadURL.c_str() );
            debugPrintf("Update %s\n", release.getUrl( Release::Download ) ); //_downloadUrlPtr );

            //t_httpUpdate_return ret = ESPhttpUpdate.update( client, _downloadURL) ;
            //t_httpUpdate_return ret = ESPhttpUpdate.update( client, _downloadUrlPtr) ;
            t_httpUpdate_return ret = ESPhttpUpdate.update( client, release.getUrl( Release::Download ));
            if ( ret == HTTP_UPDATE_OK ) {
                release.has = ! release.has;
                //stringClean();
                release.clean();
                return ! release.has;
            } else {
                debugPretty;
                debugPrintf("Error: %d\n", ret );
            }
        }
        return release.has;
    };

    String Error(){
        String out;
        switch (_lastErrorCode) {
            case Errors::Ok:
            break;

            default:
            out += _lastErrorCode;
        }
        return out; //gitHubUpgrade->getLastError();
    };

//void tick( FastBot2& bot,const BotSettings::Settings& settings){

void tick(){
    if ( settings.hasAdmin() && check() ){
        unsigned long oldUpgradeMenuId = menuIds.getUpgradeId(settings.getAdminId());
        // нужно для успешного удаления
        bot.tickManual();
        if ( oldUpgradeMenuId != 0 ) {    
            debugPrintf("Delete old menu id=%lu in admin chat %lld\n", oldUpgradeMenuId, settings.getAdminId() );
            auto res = bot.deleteMessage(settings.getAdminId(),  oldUpgradeMenuId );
            //if ( !res.valid() ) res = bot.deleteMessage(settings.getAdminId(),  oldUpgradeMenuId, false);
            //bot.tickManual();
            debugBotResult(res, "Delete old menu");
        }

      //fb::InlineMenu menu(F("Обновить;Пропустить"), F("up;ig"));
      fb::InlineMenu menu;
      menu.addButton(F("Обновить"), F("up"));
      if ( ! GitHubUpgrade::release.getUrl( Release::Info ).isEmpty() ){ //_InfoURL.isEmpty() ) {
        menu.addButton(F("Подробности"), GitHubUpgrade::release.getUrl( Release::Info )); // GitHubUpgrade::_InfoURL);
    //   if ( _InfoUrlPtr != nullptr ){
    //     menu.addButton(F("Подробности"), GitHubUpgrade::_InfoUrlPtr );
      }
      menu.addButton(F("Пропустить"), F("ig"));

      String buf(F("Текущая версия `"));
      buf += version.toString(); buf += F("`\n");
      buf +=  F("Новая версия `"); buf +=  GitHubUpgrade::tag(); buf += F("` доступна"); 

      {
        fb::Message msg(buf.c_str(), settings.getAdminId());
        msg.setModeMD();    
        msg.setInlineMenu(menu);
        //String tag = GitHubUpgrade::tag();
        
        auto res = bot.sendMessage( msg, true );
        menuIds.setUpgradeId( settings.getAdminId(), bot.lastBotMessage());
        //bot.tickManual();
        debugBotResult(res, msg.text );
      } 
      // bot.sendMessage(msg);
      
      //Serial.println( res.getRaw() );
    }

    if ( GitHubUpgrade::needUpgrade && GitHubUpgrade::release.has ) {
      //String tag = GitHubUpgrade::tag();
      //LastMsg upgradeButton(settings.getAdminId(),0, tag.c_str());
      //menuIds.( String("up")+ settings.getAdminId() );  

      String txt(START_UPGRADE);
      if ( settings.hasAdmin() ) {
        {
        fb::Message msg(txt, settings.getAdminId() );
        bot.sendMessage( msg );
        }
        unsigned long startUpMsgId = bot.lastBotMessage();

        // fb::TextEdit editMsg(txt, upgradeButton.get(), settings.getAdminId());
        // bot.editText(editMsg);
        //bot.tickManual();
      
        GitHubUpgrade::needUpgrade = false;
        
        bool done = GitHubUpgrade::doIt();
        if ( ! done ){
            txt = GitHubUpgrade::Error(); 
        } else {
            //String tag = GitHubUpgrade::tag();
            unsigned long upgradeButtonId = menuIds.getUpgradeId( settings.getAdminId() ); 
            if ( upgradeButtonId != 0 ){
                debugPrintf("Delete msg=%lu in admin chat=%lld\n", upgradeButtonId, settings.getAdminId());
                
                //fb::Result delete(){ return bot.deleteMessage( settings.getAdminId(), upgradeButton.get(), true); };
                fb::Result res;
                res = bot.deleteMessage( settings.getAdminId(), upgradeButtonId, false);
                //bot.tickManual();
                debugBotResult(res,"Delete upgrade menu");

                //menuIds.update();
            }
            txt = DONE_UPGRADE;
            //txt += REBOOT; 
            //bot.reboot();
        }
        debugPrintf("Txt=%s, to msgId=%lu\n", txt.c_str(), startUpMsgId );
        if( startUpMsgId) {
                fb::TextEdit editMsg(txt, startUpMsgId, settings.getAdminId());
                bot.editText(editMsg);
                debugPrintf("Txt:%s, msgId=%lu, chatId=%lld\n", editMsg.text.c_str(), editMsg.messageID, editMsg.chatID.toInt64() );
                
            }  
        if ( done ){
            //delay(500);
            bot.skipNextMessage();
            bot.tickManual();
            //bot.reboot();
            Serial.print("Ask reboot...");
            Serial.flush();
            ESP.restart();
        } 
        // #if defined CLEANING
        // gitHubUpgrade.clean(); 
        // #endif
      }
    }
};

}; //namespace

// bool GitHunUpgrade.checkUpgrade();
// bool GitHunUpgrade.doUpgrade();
// String GitHunUpgrade.getUpgradeURL();
// String GitHunUpgrade.getLastError();


