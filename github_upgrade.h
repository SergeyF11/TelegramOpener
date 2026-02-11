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
//extern CertStore * certStore;
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
        //static const char format[] PROGMEM = "%4d-%02d-%02dT%2d:%02d:%02dZ";
        static const char format[] = "%4d-%02d-%02dT%2d:%02d:%02dZ";  // RAM    
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
    
//     void copyUrl(char** dest, const char* src) {
//     // 1. Удаляем старый буфер (если есть)
//     delete[] *dest;
//     *dest = nullptr;
    
//     // 2. Если нечего копировать — выходим
//     if (!src || src[0] == '\0') return;
    
//     // 3. Выделяем память с учётом завершающего нуля
//     size_t len = strlen(src);
//     *dest = new char[len + 1];
//     if (*dest) {
//         strcpy(*dest, src);
//     }
//     // при неудаче new — останется nullptr, копирования не будет
// }
    
    static struct Release {
        private:
     // --- Поля (теперь String) ---
    String tag = "0.0.0dbg";
    String _downloadUrl;
    String _newCertsStore;
    String _infoUrl;
    time_t _newCertStoreDate = 0;
    bool has = false;
    bool constructed[3] = {false};
        
    public:

    enum Url { Download, Info, CertStore };

    // ==== Публичный интерфейс ====

    // Установка флага "URL может быть сконструирован"
    void setConstructed(Url type, bool value) {
        if (type < 3) constructed[type] = value;
    }

    // Получение флага
    bool isConstructed(Url type) const {
        return (type < 3) ? constructed[type] : false;
    }

    // Деструктор и запрет копирования
    ~Release() { clean(); }
    Release() = default;
    Release(const Release&) = delete;
    Release& operator=(const Release&) = delete;

    // Безопасное присвоение тега (копирует строку с контролем длины)
    void setTag(const char* src) {
        if (src && src[0]) tag = src;
        else tag = "0.0.0dbg";
    }

    const char* getTag() const { return tag.c_str(); }

    // Дата хранилища сертификатов
    void setCertStoreDate(time_t date) { _newCertStoreDate = date; }
    time_t getCertStoreDate() const { return _newCertStoreDate; }

    // Флаг наличия новой версии
    bool hasNewVersion() const { return has; }
    void setHasNewVersion(bool v) { has = v; }

    void resetCertStoreDate() { _newCertStoreDate = 0; }

    time_t getNewCertStoreDate() const {
        if (!constructed[Url::CertStore]) return 0;  // не сбрасываем дату!
        return _newCertStoreDate;
    }

    // Безопасная очистка одного указателя
    // static void _clean(char** ptr) {
    //     delete[] *ptr;
    //     *ptr = nullptr;
    // }

    // Полная очистка
    void clean() {
        has = false;
        for (int i = 0; i < 3; ++i) constructed[i] = false;
        _downloadUrl = String();
        _infoUrl = String();
        _newCertsStore = String();
        tag = "0.0.0dbg";
    }
    
    // void clean() {
    //     has = false;
    //     for (int i = 0; i < 3; ++i) constructed[i] = false;
    //     _clean(&_downloadUrl);
    //     _clean(&_infoUrl);
    //     _clean(&_newCertsStore);
    //     delete[] tag;      // <-- добавить
    //     tag = nullptr;
    // }

     // Установка кастомного URL (просто присваивание String)
    void setCustomUrl(Url type, const char* url) {
        if (!url || type >= 3) return;
        String* target = nullptr;
        switch (type) {
            case Download:  target = &_downloadUrl; break;
            case Info:      target = &_infoUrl;     break;
            case CertStore: target = &_newCertsStore; break;
        }
        if (target) *target = url;
    }

    // Получение кастомного URL (сырой указатель, только для чтения)
    const char* getCustomUrl(Url type) const {
        if (type >= 3) return nullptr;
        switch (type) {
            case Download:  return _downloadUrl.c_str();
            case Info:      return _infoUrl.c_str();
            case CertStore: return _newCertsStore.c_str();
        }
        return nullptr;
    }


    String constructUrl( Url typeUrl) const {
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

    // Проверка, можно ли сконструировать URL
    bool canConstruct(const char* url, Url typeUrl) const {
        if (!url) return false;
        String constructed(constructUrl(typeUrl));
        return constructed.equals(url);
    }

    // Получение URL (безопасно)
    // String getUrl(Url typeUrl) const {
    //     if (constructed[typeUrl]) return constructUrl(typeUrl);
    //     const char* ptr = nullptr;
    //     switch (typeUrl) {
    //         case Url::Download: ptr = _downloadUrl; break;
    //         case Url::CertStore: ptr = _newCertsStore; break;
    //         case Url::Info:      ptr = _infoUrl; break;
    //     }
    //     return ptr ? String(ptr) : String();
    // }
    String getUrl(Url typeUrl) const {
        if (isConstructed(typeUrl)) return constructUrl(typeUrl);
        const char* ptr = getCustomUrl(typeUrl);
        return ptr ? String(ptr) : String();
    }

} release;

    static bool needUpgrade = false;

    static constexpr char _sun[] PROGMEM ="Sun";
    static constexpr char _mon[] PROGMEM ="Mon";
    static constexpr char _tue[] PROGMEM ="Tue";
    static constexpr char _wed[] PROGMEM ="Wed";
    static constexpr char _thu[] PROGMEM ="Thu";
    static constexpr char _fri[] PROGMEM ="Fri";
    static constexpr char _sat[] PROGMEM ="Sat";
    static constexpr char _anyStr[] PROGMEM ="Any";
    static const char * const _weekDays[] PROGMEM = { _sun, _mon, _tue, _wed, _thu,_fri, _sat};

    const char * weekDayStr(uint day){ 
        if (day > 6) return _anyStr;   // защита от некорректного индекса
        return _weekDays[day];
    };
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
        
        bool setDay(const char* cd) {
            if (!cd || strlen(cd) < 3) return false;
            for (int i = WeekDays::Sun; i <= WeekDays::Sat; ++i) {
                if (strncmp_P(cd, (PGM_P)pgm_read_ptr(&_weekDays[i]), 3) == 0) {
                    weekDay = i;
                    return true;
                }
            }
            return false;
        }

        bool isAny(const int val){
            return val == WeekDays::Any;
        };

        // В addToString - проверка диапазона перед вызовом weekDayStr
        bool addToString(String& s, const int val, const char* (*getStr)(uint) = nullptr) {
            bool res = isAny(val);
            if (res) {
                s += FPSTR(_anyStr);
            } else if (getStr == nullptr) {
                if (val < 10) s += '0';
                s += val;
            } else {
                if (val >= 0 && val <= 6)   // защита
                    s += getStr(val);
                else
                    s += FPSTR(_anyStr);
            }
            return res;
        }


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
            //debugPretty;
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
                        //doc[su::SH("tag_name")].toStr( (char *)release.tag, sizeof( release.tag ) ); //_releaseTag);  //toString();
                        //String release_name = doc["name"].toString();
                        // String tagStr = doc[su::SH("tag_name")].toString();
                        // release.setTag(tagStr.c_str()); 
                        const char * tagStr = doc[su::SH("tag_name")].c_str();
                        release.setTag( tagStr );

                        bool prerelease = doc[su::SH("prerelease")].toBool();
                        if ( prerelease ) {
                            _lastErrorCode = Errors::PreRelease_Version;
                        } else if ( doc.has(su::SH("assets")) && doc[su::SH("assets")].isArray() ){ //&& doc["assets"].isArray() ){
                            
                            //bool valid_asset = false;
                            //preset error code
                            _lastErrorCode = Errors::No_Valid_Binary; 
                            int i = 0;
                            {
                                const String url = doc[su::SH("html_url")].toString();
                                
                                bool canBeConstructed = release.canConstruct(url.c_str(), Release::Info);
                                release.setConstructed(Release::Info, canBeConstructed);

                                if (!canBeConstructed) {
                                    release.setCustomUrl(Release::Info, url.c_str());
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
                                    const String created = asset[su::SH("created_at")].toString();

                                    release.setCertStoreDate(CertStoreFiles::getDate(created.c_str()));
                                    debugPrintf("Certstore date=%s\n", Time::toStr(release.getCertStoreDate()));

                                    const String url = asset[su::SH("browser_download_url")].toString();
                                    
                                    bool canBeConstructed = release.canConstruct(url.c_str(), Release::CertStore);
                                    release.setConstructed(Release::CertStore, canBeConstructed);
                                    if (!canBeConstructed) {
                                        release.setCustomUrl(Release::CertStore, url.c_str() );
                                    } else {
                                        debugPrintln("CertStore download url can be constructed");
                                    }
                                    // release._newCertStoreDate = CertStoreFiles::getDate( created );
                                    // debugPrintf("Certstore date=%s\n", Time::toStr( release._newCertStoreDate ));

                                    // auto url = asset[su::SH("browser_download_url")].c_str();

                                    // release.constructed[Release::CertStore] = release.canConstruct( url, Release::CertStore );
                                    // if ( ! release.constructed[Release::CertStore] ){
                                    //     //copyUrl( &_downloadUrlPtr, doc["assets"][i]["browser_download_url"].c_str());
                                    //     copyUrl( &release._newCertsStore, url);
                                    // } else {
                                    //     debugPrintln("CertStore download url can be constructed");
                                    // }
                                }
 
                                
                                if( asset[su::SH("content_type")].toString().equals( contentType) &&
                                    asset[su::SH("name")].toString().equals( App::getBinFile() ) )
                                {
                                    const String url = asset[su::SH("browser_download_url")].toString();
                                    bool canBeConstructed = release.canConstruct(url.c_str(), Release::Download);
                                    release.setConstructed(Release::Download, canBeConstructed);
                                    if (!canBeConstructed) {
                                        release.setCustomUrl(Release::Download, url.c_str() );
                                    } else {
                                        debugPrintln("ino.bin download url can be constructed");
                                    }
                                    // {
                                //     auto url = asset[su::SH("browser_download_url")].c_str();
                                //     release.constructed[Release::Download] = release.canConstruct( url, Release::Download );
                                //     if ( ! release.constructed[Release::Download] ){
                                //         //copyUrl( &_downloadUrlPtr, doc["assets"][i]["browser_download_url"].c_str());
                                //         copyUrl( &release._downloadUrl, url);
                                //     } else {
                                //         debugPrintln("ino.bin download url can be constructed");
                                //     }
                                    
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
        if (release.hasNewVersion()) {
            App::Version gitHubV(release.getTag());
            debugPrintf("GitHub newest version is %s\n", gitHubV.toString().c_str());

            if ( version >= gitHubV ) { // version <=
                //release.has = false;
                debugPrintf("Current version %s is %s the GitHub version %s\n", 
                    version.toString().c_str(), 
                    (version == gitHubV) ? "equals" : "higher",
                    gitHubV.toString().c_str());
                release.clean();
                return false;

            } 
            
            if ( menuIds.hasIgnoreVersion() ){
                App::Version ignoreVersion(menuIds.getIgnoreVersion());
                if ( ignoreVersion >= gitHubV  ){ //>=
                    //release.has = false;
                    debugPrintf("Ignore version up to %s\n", 
                        ignoreVersion.toString().c_str());  
                    release.clean();    
                    return false;  
                }
            }
        }
        return true;
    };

    bool check(bool now=false){
        if ( ! now ) 
            if ( at.checkedDay() || ! at.isTime() ) return false;
        
        debugPrintln( "Check upgrade" );
        //release.clean();
        getGitHubRelease();
        if ( _lastErrorCode == Errors::Ok ) {
                //release.has = true;
                release.setHasNewVersion(true);
                auto now = time( nullptr);
                at.checkedDay( &now );
                if ( checkVersion() ) {
                    debugPretty; 
                    debugPrintln( release.getTag() ); 
                }
        } else {
            debugPretty;
            debugPrint("Error:");
            debugPrintln( _lastErrorCode );                    
        } 
        return release.hasNewVersion();
    };
    // bool check(){
    //     if ( at.checkedDay() || ! at.isTime() ) return false;
    //     return check(true);
    // };
    inline const char * tag() {
        return release.getTag();
        // if ( release.hasNewVersion() ) return release.getTag();
        // return PSTR("");
    };
    // String tag(){
    //     if ( release.has ) return String( release.tag ); //_releaseTag ); //gitHubUpgrade->getLatestTag(); //latestTag;
    //     return NULL_STR;
    // };

    bool doIt(){
        if ( release.hasNewVersion() ){
            
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
                return ! release.hasNewVersion();
            } else {
                debugPretty;
                debugPrintf("Error: %d\n", ret );
            }
        }
        return release.hasNewVersion();
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

    if ( GitHubUpgrade::needUpgrade && GitHubUpgrade::release.hasNewVersion() ) {
      //String tag = GitHubUpgrade::tag();
      //LastMsg upgradeButton(settings.getAdminId(),0, tag.c_str());
      //menuIds.( String("up")+ settings.getAdminId() );  

      String txt(START_UPGRADE);
      if ( settings.hasAdmin() ) {
        {
        fb::Message msg(txt, settings.getAdminId() );
        msg.mode = fb::Message::Mode::MarkdownV2;
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
                editMsg.mode = fb::Message::Mode::MarkdownV2;
                bot.editText(editMsg);
                debugPrintf("Txt:%s, msgId=%lu, chatId=%s\n", editMsg.text.c_str(), editMsg.messageID, ((Text)editMsg.chatID).toString().c_str() );
                
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

bool CertStoreFiles::hasNewestCertsStore( ) {
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
    setNewCerts.text += REBOOT; //TelegramMD::asItallic( REBOOT, MARKDOWN_TG::escape );

    setNewCerts.chatID  = toId;

    auto res = bot.editText(setNewCerts, wait );
    bot.tickManual();

    debugPrintf("Send msg to %lld: %s\n", toId, setNewCerts.text.c_str() );
    return res.valid();
}