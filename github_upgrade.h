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


extern BotSettings::Settings settingsNew;
extern CertStore * certStore;
extern FastBot2Client bot;
extern App::Version version;
extern MenuIds menuIds;
extern WiFiClientSecure client;

//static String gitVersion = version.toString();
//static String gitBinFile = App::getBinFile();
//const static char certs_ar[] PROGMEM ="certs.ar";

// extern CertStore* certStore;
// BearSSL::X509List *trustedGitHubRoot;
// ESPOTAGitHub *gitHubUpgrade;

// class Url : public String {
//     public:
//     Url(){};
//     Url(String& s){
//         this->_this = _preSlash( s.c_str() );
//     };
//     private:
//     String _this;
//     String _preSlash(const char * s){
//         String out;
//         if( s != nullptr && s[0] != '/' ){
//             out += '/';
//         }
//         out += s;
//         return out;
//     };
//     String operator +=(){

//     };
// };



namespace GitHubUpgrade {
    static const char apiHost[] PROGMEM = "api.github.com";
    static const char latest[] PROGMEM = "/releases/latest";
    static const int port = 443;
    static const char contentType[] PROGMEM = "application/octet-stream";


    enum Errors {
        Ok,
        No_New_Version,
        Failed_Connection,
        No_Valid_Binary,
        No_Tag_Name,
        Failed_JSON_Parse,
        PreRelease_Version,
    };
    static Errors _lastErrorCode;
    char _releaseTag[] = "0.0.0dbg";

    //static const String appBinFile = App::getBinFile();
    

    static const time_t _likeRealTime =  2*24*60*60;
    static bool has = false;
    // static int checkedDay=0;
    //static String latestTag;
    static bool needUpgrade = false;
    static String _downloadURL = (char *)nullptr;
    static String _releaseInfoURL = (char *)nullptr;

    struct At {
        int weekDay=5;
        int hour=4;
        int min=0;
        int _checkedDay=0;
 
        bool checkedDay(const time_t* setDay=nullptr){ 
            const time_t now = time(nullptr);
            if ( now < _likeRealTime ) return true;

            const tm* nowTime =localtime(&now);

            if ( setDay != nullptr){
                auto setTime = localtime(setDay);
                _checkedDay = setTime->tm_yday;
            } 
            // else {
            //     debugPrintf("Checked day = %d, now = %d\n", _checkedDay, nowTime->tm_yday);
            // }
            return _checkedDay == nowTime->tm_yday;
        };        

        At* set( const int _weekDay=5, const int _hour=4, const int _min=0 ) /*:
            weekDay(weekDay), hour(hour), min(min) */ {
        //at = At{weekDay, hour, min};
            weekDay = _weekDay;
            hour = _hour;
            min = _min;
            return this;
        };
        static const int Any = -1;
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
    
    // enum SecureConnections {
    //     insecureConnection = 0,
    //     x509ListConnection,
    //     certsStoreConnection, 
    //     autoTypeConnection,
    // };
 
                   
    void checkAt(const int weekDay=5, const int hour=4, const int min=0 ){
        at.set(weekDay, hour, min); 
    };

    bool check(){
        if( at.checkedDay() || ! at.isTime() ) return false;
        //if ( at.isTime() ) {   
        if ( ! has ) {
            String url( F("/repos")); 
            Url::slash( url, Author::gitHubAka);
            Url::slash( url, App::name );
            Url::slash( url, latest );
            HTTPClient http;
            if ( http.begin(client, apiHost, port, url, /*https=*/true )){
                    
                int httpCode = http.GET();
                debugPretty;
                debugPrintf("Get %s:%d %s\n\tResult: %d\n", apiHost, port, url.c_str(), httpCode);
                
                if ( httpCode == HTTP_CODE_OK ){
                    
                    //debugPrintf("getString: \'%s\'\n", http.getString().c_str() );
                    gson::Parser doc;
                    if ( doc.parse( http.getString() ) && doc.has("tag_name")){
                        doc["tag_name"].toStr(_releaseTag);  //toString();
                        String release_name = doc["name"].toString();
                        bool prerelease = doc["prerelease"].toBool();
                                                    
                        if ( doc.has("assets") && doc["assets"].isArray() ){ //&& doc["assets"].isArray() ){
                            int i = 0;
                            bool valid_asset = false;
                            
                            while(  doc["assets"][i].isObject() ){
                            
                                String asset_type = doc["assets"][i]["content_type"].toString();
                                String asset_name = doc["assets"][i]["name"].toString();
                                //String asset_url =  doc["assets"][i]["browser_download_url"].toString();
                                //releaseNote = doc["name"].toString();
                                    
                                if (strcmp(asset_type.c_str(), contentType) == 0 && strcmp(asset_name.c_str(), App::getBinFile().c_str() ) == 0) {
                                    _downloadURL = doc["assets"][i]["browser_download_url"].toString();
                                    _releaseInfoURL = doc["html_url"].toString();
                                    valid_asset = true;
                                    break;
                                } else {
                                    valid_asset = false;
                                }
                                i++;
                            }
                            //doc.reset();
                            if (valid_asset) {
                                _lastErrorCode = Errors::Ok;
                                //result = true;
                                //return true;
                            } else {
                                _lastErrorCode = Errors::No_Valid_Binary; //F("No valid binary found for latest release.");
                                //return false;
                            }
                        } else {
                            _lastErrorCode = Errors::No_Tag_Name;
                        }
                    } else {
                    _lastErrorCode = Errors::Failed_JSON_Parse;
                    }

                } else {
                    _lastErrorCode = Errors::Failed_Connection;
                }
            } else {
                debugPrint("Prechecked version ");
                debugPrintln( _releaseTag ); //gitHubUpgrade->getLatestTag() );
            }

            if ( _lastErrorCode == Errors::Ok ) {
                has = true;
            } else {
                debugPretty;
                debugPrint("Error:");
                debugPrintln( _lastErrorCode );    
                return false;
            } 

            auto now = time( nullptr);
            at.checkedDay( &now );
            
            App::Version gitHubV( _releaseTag ); //gitHubUpgrade->getLatestTag());
            debugPrintf("GitHub newest version is %s\n", gitHubV.toString().c_str());

            if ( ! ( gitHubV > version ) ) { // version <=
                has = false;
                debugPrintf("Current version %s is higher that GitHub version %s\n", 
                    version.toString().c_str(), 
                    gitHubV.toString().c_str());

            } else if ( menuIds.has("ignore") ){
                App::Version ignoreVersion(menuIds.get("ignore"));
                if ( ! ( ignoreVersion < gitHubV ) ){
                    has = false;
                    debugPrintf("Ignore version up to %s\n", 
                    ignoreVersion.toString().c_str());    
                }
            }

            if ( has ) {
                // latestTag = GitHubUpgrade.getLatestTag();
                debugPretty; 
                debugPrintln( _releaseTag ); //gitHubUpgrade->getLatestTag() );
                // downloadUrl =  GitHubUpgrade.getUpgradeURL();
                
            } else {    
                debugPretty;
                // error = GitHubUpgrade.getLastError();
                debugPrint("Error:" );// gitHubUpgrade->getLastError());
                debugPrintln( _lastErrorCode );
            }
        }
        return has;
//    }

    //     return has;
    // } else 
    //     return false;
    };

    String tag(){
        if ( has ) return String(_releaseTag ); //gitHubUpgrade->getLatestTag(); //latestTag;
        return NULL_STR;
    };

    bool doIt(){
        if ( has ){
            
            ESPhttpUpdate.setClientTimeout(8000);
            ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
            ESPhttpUpdate.rebootOnUpdate(false);
            ESPhttpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

            debugPretty;
            debugPrintf("Update %s\n", _downloadURL.c_str() );

            t_httpUpdate_return ret = ESPhttpUpdate.update( client, _downloadURL) ;
            if ( ret == HTTP_UPDATE_OK ) {
                has = ! has;
                return ! has;
            } else {
                debugPretty;
                debugPrintf("Error: %d\n", ret );
            }
        }
        return has;
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

//void tick( FastBot2& bot,const BotSettings::Settings& settingsNew){
void tick(){
    if ( check() &&  settingsNew.hasAdmin() ){
     if ( menuIds.getUpgradeId(settingsNew.getAdminId()) != 0 ) {

        debugPrintln("Delete old menu");
        auto res = bot.deleteMessage(settingsNew.getAdminId(),  menuIds.getUpgradeId(settingsNew.getAdminId()));
        debugBotResult(res, "Delete old menu");
      }

      //fb::InlineMenu menu(F("Обновить;Пропустить"), F("up;ig"));
      fb::InlineMenu menu;
      menu.addButton(F("Обновить"), F("up"));
      if ( ! GitHubUpgrade::_releaseInfoURL.isEmpty() ) {
        menu.addButton(F("Подробности"), GitHubUpgrade::_releaseInfoURL);
      }
      menu.addButton(F("Пропустить"), F("ig"));

      String buf(F("Текущая версия `"));
      buf += version.toString(); buf += F("`\n");
      buf +=  F("Новая версия `"); buf +=  GitHubUpgrade::tag(); buf += F("` доступна"); 

      {
        fb::Message msg(buf.c_str(), settingsNew.getAdminId());
        msg.setModeMD();

        debugPrintln(msg.chatID);
        debugPrintMemory;
        
        msg.setInlineMenu(menu);
        debugPrintMemory;
        //String tag = GitHubUpgrade::tag();
        
        auto res = bot.sendMessage( msg, true );
        menuIds.setUpgradeId( settingsNew.getAdminId(), bot.lastBotMessage());
        debugPrintln( msg.text );
      } 
      // bot.sendMessage(msg);
      bot.tickManual();
      //Serial.println( res.getRaw() );
    }

    if ( GitHubUpgrade::needUpgrade && GitHubUpgrade::has ) {
      //String tag = GitHubUpgrade::tag();
      //LastMsg upgradeButton(settingsNew.getAdminId(),0, tag.c_str());
      //menuIds.( String("up")+ settingsNew.getAdminId() );  

      String txt(F("Start upgrade..."));
      if ( settingsNew.hasAdmin() ) {
        fb::Message msg(txt, settingsNew.getAdminId() );
        bot.sendMessage( msg );
        unsigned long startUpMsgId = bot.lastBotMessage();

        // fb::TextEdit editMsg(txt, upgradeButton.get(), settingsNew.getAdminId());
        // bot.editText(editMsg);
        bot.tickManual();
      
        GitHubUpgrade::needUpgrade = false;
        
        bool done = GitHubUpgrade::doIt();
        if ( ! done ){
            txt = GitHubUpgrade::Error(); 
        } else {
            //String tag = GitHubUpgrade::tag();
            unsigned long upgradeButtonId = menuIds.getUpgradeId( settingsNew.getAdminId() ); 
            if ( upgradeButtonId != 0 ){
                debugPrintf("Delete msg=%lu in chat=%llu\n", upgradeButtonId/* upgradeButton.get() */, settingsNew.getAdminId());
                
                //fb::Result delete(){ return bot.deleteMessage( settingsNew.getAdminId(), upgradeButton.get(), true); };
                fb::Result res;
                res = bot.deleteMessage( settingsNew.getAdminId(), upgradeButtonId, true);
                debugBotResult(res,"Delete upgrade menu");

                menuIds.update();
            }
            txt = F("Upgrade done. Reboot...");
            //bot.reboot();
        }
        debugPrintf("Txt=%s, to msgId=%lu\n", txt.c_str(), startUpMsgId );
        if( startUpMsgId) {
                fb::TextEdit editMsg(txt, startUpMsgId, settingsNew.getAdminId());
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


