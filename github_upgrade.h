#pragma once
#include <ESP_OTA_GitHub.h>
#include "env.h"
#include <time.h>
#include <FastBot2.h>
#include "newFsSettings.h"
#include "LastMsgInFile.h" 
#include "env.h"

extern BotSettings::Settings settingsNew;
extern FastBot2 bot;
extern App::Version version;

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
    // static String downloadUrl;
    // static String error;
    ESPOTAGitHub GitHubUpgrade( 
                Author::gitHubAka, 
                App::name, gitVersion.c_str(), 
                gitBinFile.c_str(), false);
                
    void setAt(const int weekDay=5, const int hour=4, const int min=0 ){
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
        if ( ( at.weekDay == -1 ||  nowTime->tm_wday == at.weekDay ) &&
            ( at.hour == -1 || nowTime->tm_hour == at.hour ) && 
            ( at.min == -1 || nowTime->tm_min == at.min )) {
            
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
      fb::InlineMenu menu("Upgrade", "up");
      //bot.tickManual();
      //char buf[50] = {0};
      //sprintf(buf, "Новая версия `%s` доступна", GitHubUpgrade::tag().c_str() );
      String buf(F("Текущая версия `"));
      buf += version.toString(); buf += F("`\n");
      buf +=  F("Новая версия `"); buf +=  GitHubUpgrade::tag(); buf += F("` доступна");
      
      {
      fb::Message msg(buf.c_str(), settingsNew.getAdminId());
      msg.setModeMD();
      //String cmd(F("up"));
      debugPrintln(msg.chatID);
      debugPrintMemory;
     
      msg.setInlineMenu(menu);
      debugPrintMemory;
      auto res = bot.sendMessage( msg, true );
      String tag = GitHubUpgrade::tag();
      LastMsg upgradeButton(settingsNew.getAdminId(), bot.lastBotMessage(), tag.c_str());
      upgradeButton.set();

      // if ( !res.valid() ){
      //   msg.removeMenu();
      //   bot.sendMessage( msg, false);
      // }
      debugPrintln( msg.text );
      }
      // bot.sendMessage(msg);
      bot.tickManual();
      //Serial.println( res.getRaw() );
    }

    if ( GitHubUpgrade::needUpgrade && GitHubUpgrade::has ) {
      String tag = GitHubUpgrade::tag();
      LastMsg upgradeButton(settingsNew.getAdminId(),0, tag.c_str());
      String txt(F("Start upgrade..."));
      if ( settingsNew.hasAdmin() ) {
        fb::Message msg("Start upgrade...", settingsNew.getAdminId() );
        bot.sendMessage( msg );
        // fb::TextEdit editMsg(txt, upgradeButton.get(), settingsNew.getAdminId());
        // bot.editText(editMsg);
        bot.tickManual();
      }
      GitHubUpgrade::needUpgrade = false;
      
      bool done = GitHubUpgrade::doIt();
      if ( ! done ){
        txt = GitHubUpgrade::Error(); 
      } else {
        String tag = GitHubUpgrade::tag();
        
        debugPrintf("Delete msg=%lu in chat=%llu\n", upgradeButton.get(), settingsNew.getAdminId());
        
        //fb::Result delete(){ return bot.deleteMessage( settingsNew.getAdminId(), upgradeButton.get(), true); };
        fb::Result res;
        while ( ! res.valid() ){
            res = bot.deleteMessage( settingsNew.getAdminId(), upgradeButton.get(), true);
            if ( res.valid() ) {
                upgradeButton.clean();
                debugPrintln();
                break;
            } else {
                debugPrint(".");
                delay(300);
            }
        }

        txt = F("Upgrade done. Reboot...");
        //bot.reboot();
      }
      debugPrintf("Txt=%s, to msgId=%lu\n", txt.c_str(), bot.lastBotMessage() );
      if( bot.lastBotMessage() ){
            fb::TextEdit editMsg(txt, upgradeButton.get(), settingsNew.getAdminId());
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
// bool GitHunUpgrade.checkUpgrade();
// bool GitHunUpgrade.doUpgrade();
// String GitHunUpgrade.getUpgradeURL();
// String GitHunUpgrade.getLastError();


