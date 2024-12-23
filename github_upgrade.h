#pragma once
#include <ESP_OTA_GitHub.h>
#include "env.h"
#include <time.h>
#include <FastBot2s.h>
#include "newFsSettings.h"
//#include "myFileDb.h"
#include "myPairs.h"

extern BotSettings::Settings settingsNew;
extern FastBot2 bot;
extern App::Version version;
extern MenuIds menuIds;

static String gitVersion = version.toString();
static String gitBinFile = App::getBinFile();


namespace GitHubUpgrade {
    
    static bool has = false;
    static int checkedDay=0;
    //static String latestTag;
    static bool needUpgrade = false;
    struct At {
        int weekDay=5;
        int hour=4;
        int min=0;
    };
    static At at; 
    static const int AnyTime = -1;
    // static String downloadUrl;
    // static String error;
    ESPOTAGitHub GitHubUpgrade( 
                Author::gitHubAka, 
                App::name, gitVersion.c_str(), 
                gitBinFile.c_str(), false);
                
    void checkAt(const int weekDay=5, const int hour=4, const int min=0 ){
        //at = At{weekDay, hour, min};
        at.weekDay = weekDay;
        at.hour = hour;
        at.min = min;
    }            
    bool check(){
        // ESPOTAGitHub GitHubUpgrade( 
        //     Author::gitHubAka, 
        //     App::name, gitVersion.c_str(), 
        //     gitBinFile.c_str(), false);
        auto now = time(nullptr);
        //if ( now < 6000ll ) return false;

        auto nowTime = localtime(&now);
        if (  now < 6000ll || nowTime->tm_yday == checkedDay ) return false;
        if ( ( at.weekDay == AnyTime ||  nowTime->tm_wday == at.weekDay ) &&
            ( at.hour == AnyTime || nowTime->tm_hour == at.hour ) && 
            ( at.min == AnyTime || nowTime->tm_min == at.min )) {
            
            if ( ! has ) 
                has = GitHubUpgrade.checkUpgrade();
            checkedDay = nowTime->tm_yday;
            //App::Version gitHubV;
            //gitHubV.fromString(GitHubUpgrade.getLatestTag() );
            
            App::Version gitHubV(GitHubUpgrade.getLatestTag());

            if ( ! ( gitHubV > version ) ) {
                has = false;
                debugPrintf("Current version %s is higher that GitHub version %s\n", 
                    version.toString().c_str(), 
                    gitHubV.toString().c_str());
            }    
            if ( has ) {
                // latestTag = GitHubUpgrade.getLatestTag();
                debugPretty; 
                debugPrintln(GitHubUpgrade.getLatestTag() );
                // downloadUrl =  GitHubUpgrade.getUpgradeURL();
               
            } else {    
                // error = GitHubUpgrade.getLastError();
                debugPrintln(GitHubUpgrade.getLastError());
            }
        }
        return has;
    }

    String tag(){
        if ( has ) return GitHubUpgrade.getLatestTag(); //latestTag;
        return NULL_STR;
    }

    bool doIt(){
        if ( has ){
            return GitHubUpgrade.doUpgrade();
        }
        return false;
    }
    String Error(){
        return GitHubUpgrade.getLastError();
        //return error;
    }

//void tick( FastBot2& bot,const BotSettings::Settings& settingsNew){
void tick(){
    if ( check() &&  settingsNew.hasAdmin() ){
     if ( menuIds.getUpgradeId(settingsNew.getAdminId()) != 0 ) {
        debugPrintln("Delete old menu");
        auto res = bot.deleteMessage(settingsNew.getAdminId(),  menuIds.getUpgradeId(settingsNew.getAdminId()));
        debugBotResult(res, "Delete old menu");
        // if ( res.valid() ){
        //     debugPrintln("deleted");
        // } else {
        //     debugPrintln( "ERROR: delete old menu");
        // }
      }

      fb::InlineMenu menu("Upgrade", "up");

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
        String tag = GitHubUpgrade::tag();
        
        auto res = bot.sendMessage( msg, true );
        menuIds.setUpgradeId( settingsNew.getAdminId(), bot.lastBotMessage());
        debugPrintln( msg.text );
      } 
      // bot.sendMessage(msg);
      bot.tickManual();
      //Serial.println( res.getRaw() );
    }

    if ( GitHubUpgrade::needUpgrade && GitHubUpgrade::has ) {
      String tag = GitHubUpgrade::tag();
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
            String tag = GitHubUpgrade::tag();
            unsigned long upgradeButtonId = menuIds.getUpgradeId( settingsNew.getAdminId() ); 
            if ( upgradeButtonId != 0 ){
                debugPrintf("Delete msg=%lu in chat=%llu\n", upgradeButtonId/* upgradeButton.get() */, settingsNew.getAdminId());
                
                //fb::Result delete(){ return bot.deleteMessage( settingsNew.getAdminId(), upgradeButton.get(), true); };
                fb::Result res;
                res = bot.deleteMessage( settingsNew.getAdminId(), upgradeButtonId, true);
                debugBotResult(res,"Delete upgrade menu");
                // if ( res.valid() ) {
                //     debugPrintln("Deleted upgrade menu");
                // } else {
                //     debugPrintln("ERROR: delete upgrade menu");
                // }

                // while ( ! res.valid() ){
                //     res = bot.deleteMessage( settingsNew.getAdminId(), upgradeButtonId /*upgradeButton.get()*/, true);
                //     if ( res.valid() ) {
                //         //upgradeButton.clean();
                //         menuIds.removeUpgradeId( settingsNew.getAdminId() );
                //         debugPrintln();
                //         break;
                //     } else {
                //         debugPrint(".");
                //         delay(300);
                //     }
                // }
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
                bot.tickManual();
            }  
        if ( done ){
            //delay(500);
            bot.skipNextMessage();
            bot.reboot();
        }
      }
    }
}
}
// bool GitHunUpgrade.checkUpgrade();
// bool GitHunUpgrade.doUpgrade();
// String GitHunUpgrade.getUpgradeURL();
// String GitHunUpgrade.getLastError();


