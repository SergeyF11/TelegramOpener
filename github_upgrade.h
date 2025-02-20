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
#include <TimeLib.h>

extern BotSettings::Settings settings;
extern CertStore * certStore;
extern FastBot2Client bot;
extern App::Version version;
extern MenuIds menuIds;
extern WiFiClientSecure client;

namespace CertStoreFiles {
    static const char dataCerts[] PROGMEM = "data/certs.ar";
    static constexpr const char * fileData PROGMEM = dataCerts+4; //"/certs.ar";
    static const char fileIdx[] PROGMEM = "/certs.idx";

    time_t fileDate(FS& fs, const char * fileName = fileData );
    bool hasNewestCertsStore();
    
    time_t getDate(const char * dateString){
        static const char format[] PROGMEM = "%4d-%02d-%02dT%2d:%02d:%02dZ";
        time_t out = 0;
        if ( dateString == nullptr || dateString[0] == '\0' ) return out;

        struct tm timeinfo;
        auto args = sscanf(dateString, format, 
                &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday, 
                &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec );
//        debugPrint(args);

        if ( args == 6 ){
            
/*         debugPrintf(" ==> Date: %2u-%02u-%4u Time: %2u:%02u:%02u\n",
            timeinfo.tm_mday, timeinfo.tm_mon, timeinfo.tm_year,
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec );
 */
            tmElements_t tmSet;
            tmSet.Year = timeinfo.tm_year-1970;
            tmSet.Month = timeinfo.tm_mon;
            tmSet.Day = timeinfo.tm_mday;
            tmSet.Hour = timeinfo.tm_hour;
            tmSet.Minute = timeinfo.tm_min;
            tmSet.Second = timeinfo.tm_sec;
            
            out = makeTime(tmSet);
            debugPrintf( "Time: %lu\n", out );
        } else {
            debugPrintf(" Error\n");
        }
        return out;
    };
    bool updatedMsg(FastBot2Client& b, const long long, bool wait = false);
    bool downloadMsg(FastBot2Client& bot, const long long toId, bool wait = true );
};

namespace GitHubUpgrade {
    static const char apiHost[] PROGMEM = "api.github.com";
    static const char latest[] PROGMEM = "/releases/latest";
    static const int port = 443;
    static const char contentType[] PROGMEM = "application/octet-stream";
    static const char archiveType[] PROGMEM = "application/x-archive";


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
    
    void copyUrl(char ** dest, const su::Text::Cstr src){ 
        #ifdef debug_print 
            debugPretty;
            debugPrint( "Dest adr=" );
            Serial.println( (long long unsigned int)&(*dest), HEX );
            debugPrintln( src );
        #endif
        if ( *dest != nullptr ) delete[] dest;
        //auto buf = src;
        *dest = new char[ strlen( src ) ]; //(char *)malloc(  );
        strcpy( *dest, src );
        debugPrintln( *dest );
    };
    
    static struct Release {
        char tag[9] = "0.0.0dbg";
        enum Url {
            Download,
            Info,
            CertStore,
        };
        char * _downloadUrl = nullptr;
        char * _newCertsStore = nullptr;
        char * _infoUrl = nullptr;
        time_t _newCertStoreDate = 0;
        void resetCertStoreDate(){
            _newCertStoreDate=0;
        };
        time_t getNewCertStoreDate(){
            if ( ! constructed[ Url::CertStore ] ) resetCertStoreDate();
            return _newCertStoreDate;
        };

        // bool  hasCertStore(){
        //     return constructed[ Url::CertStore ] && 
        //      _newCertStoreDate != 0;
        // };


        bool has = false;
        bool constructed[3] = {false};
        void _clean( char ** ptr){
            if ( *ptr != nullptr ){
                delete[] *ptr;
                *ptr = nullptr;
            }
        };
        void clean(){
            debugPretty;
            debugPrintf("%s download=%s, info=%s\n", has ? "has" : "none", 
                _downloadUrl == nullptr ? "nullptr" : _downloadUrl,
                _infoUrl == nullptr ? "nullptr" : _infoUrl);
            has = false;
            _clean( &_downloadUrl);
            _clean( &_infoUrl );
            _clean( &_newCertsStore );

            // if ( _downloadUrl != nullptr ){
            //     delete[](_downloadUrl);
            //     _downloadUrl = nullptr;
            // }
            // if ( _infoUrl != nullptr  ){
            //     delete[](_infoUrl);
            //     _infoUrl = nullptr; 
            // }
        };


        String constructUrl( Url typeUrl){
            String out = App::getHomePage();
            out += latest;
            out = out.substring(0, out.length()-6);
            if ( typeUrl == Url::Info ) {
                    out += F("tag/");
                    out += tag;
            } else {
                out += F("download/");
                out += tag;
                if ( typeUrl == Url::Download ) {
                    out += '/';
                    out += App::getBinFile();
                } else {
                    out += CertStoreFiles::fileData;
                }
            }            
            // debugPretty;
            // debugPrintln(out);
            return out;
        };
        bool canConstruct( const char * url, Url typeUrl ){
            String constructed( constructUrl( typeUrl));
            return constructed.equals( url ); 
        };
        
        String getUrl(Url typeUrl){
            if ( constructed[typeUrl] ) return constructUrl(typeUrl);
            // else
            String out;
            switch (typeUrl){
                case Url::Download: 
                    out = String(_downloadUrl);
                    break;
                case Url::CertStore:
                    out = _newCertsStore;
                    break;
                case Url::Info:
                    out = _infoUrl;
                    break;    
            }
            return out; //String( typeUrl == Url::Download ? _downloadUrl : _infoUrl );
        };
       
    }/* ;
    static Release */ release;
    static bool needUpgrade = false;

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
                    break;
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
            bool res;
            {
                const time_t now = time(nullptr);
                const tm* nowTime =localtime(&now);

                res = _checkedDay == nowTime->tm_yday;
            }
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
            bool _isTime;
            {
            const time_t now = time(nullptr);
            auto nowTime = localtime(&now);
            //if ( checkedDay(nowTime) ) return false;
            //if ( now == 0 ) now = time(nullptr);
            
            _isTime = ( weekDay == Any ||  nowTime->tm_wday == weekDay ) &&
                ( hour == Any || nowTime->tm_hour == hour ) && 
                ( min == Any || nowTime->tm_min == min );
            }
            return _isTime;
        };
    };
    static At at; 
    
                   
    // void checkAt(const int weekDay=5, const int hour=4, const int min=0 ){
    //     at.set(weekDay, hour, min); 
    // };

    Errors getGitHubRelease(){
        HTTPClient http;
        String url( F("/repos")); 
        Url::slash( url, Author::gitHubAka);
        Url::slash( url, App::name );
        Url::slash( url, latest );
        
        if ( ! http.begin(client, apiHost, port, url, /*https=*/true )) {
            _lastErrorCode = Errors::Failed_Connection;
        } else {  
            delay(0);
            http.setTimeout(1500);
            int httpCode = http.GET();
            debugPretty;
            debugPrintf("Get %s:%d %s\n\tResult: %d\n", apiHost, port, url.c_str(), httpCode);
            
            if ( httpCode != HTTP_CODE_OK ) { 
                _lastErrorCode = Errors::Failed_Host_Response;    
            } else {   
                // 200 Ok => parsing response
              
                gson::Parser doc; //(100);

                if ( ! doc.parse( http.getString() ) ) {
                    _lastErrorCode = Errors::Failed_JSON_Parse;
                } else {
                    doc.hashKeys();
                    if( ! doc.has(su::SH("tag_name")) ){
                        _lastErrorCode = Errors::No_Tag_Name;
                    } else {
                        doc[su::SH("tag_name")].toStr( (char *)release.tag ); //_releaseTag);  //toString();
                        //String release_name = doc["name"].toString();
                        bool prerelease = doc[su::SH("prerelease")].toBool();
                        if ( prerelease ) {
                            _lastErrorCode = Errors::PreRelease_Version;
                        } else if ( doc.has(su::SH("assets")) && doc[su::SH("assets")].isArray() ){ //&& doc["assets"].isArray() ){
                            
                            //bool valid_asset = false;
                            //preset error code
                            _lastErrorCode = Errors::No_Valid_Binary; 
                            int i = 0;
                            {
                                auto url = doc[su::SH("html_url")].c_str();
                                release.constructed[Release::Info] = release.canConstruct( url, Release::Info );

                                if ( ! release.constructed[Release::Info] ){
                                    copyUrl( &release._infoUrl, url );
                                    //copyUrl( &_InfoUrlPtr, doc["html_url"].c_str());
                                } else {
                                    debugPrintln(F("Info url can be constructed"));
                                }  
                            }
                            while( true ){
                                auto asset = doc[su::SH("assets")][i];
                                if ( ! asset.isObject() ) break;

                                if( asset[su::SH("content_type")].toString().equals( archiveType ) &&
                                    asset[su::SH("name")].toString().equals( CertStoreFiles::fileData+1 ) )
                                {
                                    auto created = asset[su::SH("created_at")].c_str();
                                    release._newCertStoreDate = CertStoreFiles::getDate( created );
                                    debugPrintf("Certstore date=%s\n", Time::toStr( release._newCertStoreDate ));

                                    auto url = asset[su::SH("browser_download_url")].c_str();
                                    release.constructed[Release::CertStore] = release.canConstruct( url, Release::CertStore );
                                    if ( ! release.constructed[Release::CertStore] ){
                                        //copyUrl( &_downloadUrlPtr, doc["assets"][i]["browser_download_url"].c_str());
                                        copyUrl( &release._newCertsStore, url);
                                    } else {
                                        debugPrintln("CertStore download url can be constructed");
                                    }
                                }
 
                                
                                if( asset[su::SH("content_type")].toString().equals( contentType) &&
                                    asset[su::SH("name")].toString().equals( App::getBinFile() ) )
                                {
                                    auto url = asset[su::SH("browser_download_url")].c_str();
                                    release.constructed[Release::Download] = release.canConstruct( url, Release::Download );
                                    if ( ! release.constructed[Release::Download] ){
                                        //copyUrl( &_downloadUrlPtr, doc["assets"][i]["browser_download_url"].c_str());
                                        copyUrl( &release._downloadUrl, url);
                                    } else {
                                        debugPrintln("ino.bin download url can be constructed");
                                    }
                                    
                                    _lastErrorCode = Errors::Ok;
                                    //break;
                                } 
                                i++;
                            }
                        }
                    }
                }
                doc.reset();
            }
        }
        http.end();
        return _lastErrorCode;
    };

    bool checkVersion(){
        if ( release.has) {
            App::Version gitHubV( release.tag ); //_releaseTag ); //gitHubUpgrade->getLatestTag());
            debugPrintf("GitHub newest version is %s\n", gitHubV.toString().c_str());

            if ( version >= gitHubV ) { // version <=
                //release.has = false;
                debugPrintf("Current version %s is %s the GitHub version %s\n", 
                    version.toString().c_str(), 
                    (version == gitHubV) ? "equals" : "higher",
                    gitHubV.toString().c_str());
                release.clean();

            } else if ( menuIds.hasIgnoreVersion() ){
                App::Version ignoreVersion(menuIds.getIgnoreVersion());
                if ( ignoreVersion >= gitHubV  ){ //>=
                    //release.has = false;
                    debugPrintf("Ignore version up to %s\n", 
                        ignoreVersion.toString().c_str());  
                    release.clean();      
                }
            }
        }
        return release.has;
    };
    bool check(bool now=false){
        if ( ! now ) 
            if ( at.checkedDay() || ! at.isTime() ) return false;
            
        release.clean();
        if ( getGitHubRelease() == Errors::Ok ) {
                release.has = true;
                auto now = time( nullptr);
                at.checkedDay( &now );
                if ( checkVersion() ) {
                    debugPretty; 
                    debugPrintln( release.tag ); 
                }
            } else {
                debugPretty;
                debugPrint("Error:");
                debugPrintln( _lastErrorCode );                    
            } 
        return release.has;
    };
    // bool check(){
    //     if ( at.checkedDay() || ! at.isTime() ) return false;
    //     return check(true);
    // };
    const char * tag(){
        if ( release.has ) return release.tag;
        return PSTR("");
    };
    // String tag(){
    //     if ( release.has ) return String( release.tag ); //_releaseTag ); //gitHubUpgrade->getLatestTag(); //latestTag;
    //     return NULL_STR;
    // };

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
                //release.has = ! release.has;
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
     
      if ( ! GitHubUpgrade::release.getUrl( Release::Info ).isEmpty() ){ //_InfoURL.isEmpty() ) {
        menu.addButton(F("Подробности"), GitHubUpgrade::release.getUrl( Release::Info )); // GitHubUpgrade::_InfoURL);
        menu.newRow();
    //   if ( _InfoUrlPtr != nullptr ){
    //     menu.addButton(F("Подробности"), GitHubUpgrade::_InfoUrlPtr );
      } 
      menu.addButton(F("Обновить"), F("up"));
      menu.addButton(F("Пропустить"), F("ig"));

      String buf(F("Текущая версия `"));
      buf += version.toString(); buf += F("`\n");
      buf +=  F("Новая версия `"); buf += GitHubUpgrade::tag(); buf += F("` доступна"); 

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
            txt += REBOOT; 
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

time_t CertStoreFiles::fileDate(FS& fs, const char * fileName ){
    time_t res = 0;
    auto f = fs.open(fileName, "r"); 
    if ( f ) {
        res = f.getLastWrite();
        f.close();
    }
    return res;
};

bool CertStoreFiles::hasNewestCertsStore( )    {
    static time_t myCertsDate = CertStoreFiles::fileDate(LittleFS );
    auto newDate = GitHubUpgrade::release.getNewCertStoreDate();
    if ( newDate == 0 ) { 
        //debugPrintln("No GitHub release checked");
        return false;
    }
    return ( newDate > myCertsDate );  
};
bool CertStoreFiles::downloadMsg(FastBot2Client& bot, const long long toId, bool wait ){
    if ( ! toId ) return false;
    bot.tickManual();
    fb::Message certsDownload( TelegramMD::asItallic( F("Обновляю сертификаты..."), MARKDOWN_TG::escape),  toId);
    certsDownload.setModeMD();
    auto res = bot.sendMessage( certsDownload, wait );
    return res.valid(); //bot.lastBotMessage();
};
bool CertStoreFiles::updatedMsg(FastBot2Client& bot, const long long toId, bool wait){
    if ( ! toId ) return false;
    bot.tickManual();
    //GitHubUpgrade::release.resetCertStoreDate();
    fb::TextEdit setNewCerts;
    setNewCerts.messageID = bot.lastBotMessage();
    setNewCerts.mode = fb::Message::Mode::MarkdownV2;
       
    setNewCerts.text = TelegramMD::asBold( F("Сертификаты обновлены."), MARKDOWN_TG::escape );
    setNewCerts.text += TelegramMD::newLine();
    setNewCerts.text += TelegramMD::asItallic(F("Требуется перезагрузка!"), MARKDOWN_TG::escape );
    setNewCerts.text += TelegramMD::asItallic( REBOOT, MARKDOWN_TG::escape );

    setNewCerts.chatID  = toId;

    auto res = bot.editText(setNewCerts, wait );
    bot.tickManual();

    debugPrintf("Send msg to %lld: %s\n", toId, setNewCerts.text.c_str() );
    return res.valid();
}