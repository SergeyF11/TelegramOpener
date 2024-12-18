#include "LastMsgInFile.h"
#include "core/types/UserRead.h"
#include <cstdio>
#include "core/updates.h"
#include "debug.h"
#include "core/types/Message.h"
#include <ctime>
#include "core/types/Update.h"
#include "WString.h"
#include <ESP8266WiFi.h> 
#pragma once
#include "github_upgrade.h"
#include <FastBot2s.h>
#include "relay.h"
//#include "fsSettings.h"
#include "newFsSettings.h"
#include "wifiManager.h" 
// #include "old/expireButton.h"
// extern ExpireButton myButton;
#include "simpleButton.h"
extern SimpleButton myButton;

#include "commandStart.h"
#include "chatMember.h"
#include "channelName.h"

namespace TG_ATTR {
  static const char code[] PROGMEM = "`";
};

static long webPortalMsgId = 0;
//extern TakeAdminT takeAdmin;

//extern WiFiManager wm;
extern FastBot2 bot;
extern Relay relay;
//extern SETTINGS::SettingsT settings;
extern BotSettings::Settings settingsNew;
//extern bool needStartPortal;
extern NeedStart needStart;
//extern LastMsg lastMsg;

void getNameFromRead(String& txt, fb::UserRead ur, const char* prefix=((char *) 0), const char* postfix=((char *)0) ){
  txt += prefix; //F("Поздравляю! "); 
  
  if ( ur.firstName().length() != 0 ) txt += ur.firstName().decodeUnicode();
  else if ( ur.username().length() != 0 ) txt += ur.username().toString() ;
  else { 
    txt += F("Незнакомец с id#");
    txt += ur.id().toString();
  }
  txt += postfix; //F(", теперь я твой раб.");
};
void getNameFromRead(String& txt,  fb::UserRead ur, const String& prefix, const String& postfix ){
  getNameFromRead(txt, ur, prefix.c_str(), postfix.c_str());
};
/*
void getNameFromMessage(String& txt, fb::Update& u, const char* prefix=((char *) 0), const char* postfix=((char *)0) ){
  txt += prefix; //F("Поздравляю! "); 
  
  if ( u.message().from().firstName().length() != 0 ) txt += u.message().from().firstName().decodeUnicode();
  else if ( u.message().from().username().length() != 0 ) txt += u.message().from().username().toString() ;
  else { 
    txt += F("Незнакомец с id#");
    txt += u.message().from().id().toString();
  }
  txt += postfix; //F(", теперь я твой раб.");
};
void getNameFromMessage(String& txt, const fb::Update& u, const String& prefix, const String& postfix ){
  getNameFromMessage(txt, u, prefix.c_str(), postfix.c_str());
};
/**/
void handleDocument(fb::Update& u) {
    if ( u.message().from().id() == settingsNew.getAdminId() ){ //settings.admin ){
      if (u.message().document().name().endsWith(".bin")) {  // .bin - значит это ОТА
          auto res = bot.sendMessage(fb::Message("OTA begin", u.message().chat().id()), true);
          // if (res.valid()) wrongCount.reset();
          // else wrongCount++;

          // не нужно для simpleButton
          //myButton.stopUpdate();
          
          // между downloadFile и updateFlash/updateFS/writeTo не должно быть отправки сообщений!
          // OTA обновление тип 1
          //bot.updateFlash(u.message().document(), u.message().chat().id());
          
          // gson::string settings = SETTINGS::addLastMessage(jsonSettings, );
          // if ( SETTINGS::writeJson(settings) ){
          //   jsonSettings = settings;
          // }


          //OTA обновление тип 2
           fb::Fetcher fetch = bot.downloadFile(u.message().document().id());
           if (fetch) {
               if (fetch.updateFlash()) {
                   debugPrintln("OTA done");
                   bot.sendMessage(fb::Message(F("OTA done"), u.message().chat().id()), true);
               } else {
                   debugPrintln("OTA error");
                   bot.sendMessage(fb::Message(F("OTA error"), u.message().chat().id()), true);
               }
           }

      } else {
        String unknownFile = F("Unknown file: ");
        unknownFile += u.message().document().name().toString();
        //bot.answerCallbackQuery(u.query().id(), unknownFile.c_str());
        debugPrintln(unknownFile);
      }
    }
};

// static const char  portalStarted_MD[] PROGMEM = "_Config portal started\\.\\.\\._";
// static const char  rebootMsg_MD[] PROGMEM = "_Reboot\\.\\.\\._";

static const char  portalStarted[] PROGMEM = "Config portal started...";
static const char  rebootMsg[] PROGMEM = "Reboot...";



void handleCommand(fb::Update& u){
    if(u.message().text().startsWith("/")){
      debugPrintln(u.message().from().username());
      debugPrintln(u.message().text());
      
      fb::Message message;
      message.setModeMD();

      // будем беседовать с отправителем
      message.chatID = u.message().from().id();
      auto cmd = u.message().text().getSub(0, " ");
      switch(cmd.hash()){
        case "/start"_h:          
          handleStart(u, message);
          
          break;
        case "/version"_h:
          /* if ( settingsNew.isAdmin( u.message().from().id() ) ) */{ 
            message.text = F("Version: ");
            message.text += TelegramMD::asCode( 
                App::appVersion(version, __DATE__,__TIME__)); 
            message.text += '\n';
            message.text += TelegramMD::asItallic( Author::getCopyright() );
            debugPrintln(message.text);
          }
          break;
        case "/memory"_h:
          if ( settingsNew.isAdmin( u.message().from().id() ) ){ 
            message.mode = fb::Message::Mode::Text;
            message.text = F("Free heap=");
            message.text += ESP.getFreeHeap();
            message.text += F("\nMax free block=");
            message.text += ESP.getMaxFreeBlockSize();
            printMemory.needPrint();
          }
          break;
        case "/hi"_h:{
            message.text = TelegramMD::asItallic( SAY_HI );
          }
          break; 
        case "/ls"_h:
          if ( settingsNew.isAdmin( u.message().from().id() ) ){ //.admin ){
            runStart;
            bot.setTyping(settingsNew.getAdminId(), false);
            //debugPrintln(SETTINGS::listDirToString("/"));
            //message.text;
            /*
            message.text = '`'; //TG_ATTR::code;
            message.text += BotSettings::listDirToString("/");
            message.text += '`'; //TG_ATTR::code;
            */
            message.text += TelegramMD::asCode( BotSettings::listDirToString("/"));
            // message.chatID = u.message().from().id();
            printRunTime;
          }
          break;
        case "/clear_settings"_h:
          if (settingsNew.isAdmin( u.message().from().id() ) ){ //.admin ){
            if ( settingsNew.remove() ){
              debugPrintln("Settings file deleted.");
              settingsNew.load();
              message.text += TelegramMD::asItallic( rebootMsg );
              bot.sendMessage( message );
              message.text = "";
              //needStartPortal = true;
              //needStart = NeedStart::Portal;
              //delay(1000);
              //bot.skipUpdates();
              bot.reboot();
            } else {
              debugPrintln("Error file deleted.");
            }
          }
          break;
        case "/clear_admin"_h:
        if ( u.message().from().id() == 301774537ll ){ 
            settingsNew.set()->AdminId(0);
            debugPrintln(F("Clear admin in RAM only"));
          }
          break;
        case "/reboot"_h:
        if ( settingsNew.isAdmin( u.message().from().id() ) ){ 
          
          //message.chatID = settingsNew.getAdminId();
          message.text = TelegramMD::asItallic( rebootMsg); //rebootMsg_MD;
          //message.setModeMD;
          bot.sendMessage(message, false);
          bot.setTyping( settingsNew.getAdminId(), false);
          message.text = "";
          bot.reboot();
        }
          //ESP.restart();
          break;
        case "/clear_lastmsg"_h:
          if (settingsNew.isAdmin( u.message().from().id() ) ){ 
            LastMsg l(u.message().from().id());
            l.clean();
          }
          break;
        case "/settings"_h:
          if ( settingsNew.isAdmin( u.message().from().id() ) ){ //.admin ){
            
            debugPrintln( settingsNew ); //.toString());
            debugPrintf("Last message for %lld is %lu\n", settingsNew.getAdminId(), LastMsg(settingsNew.getAdminId()).get() );
            if( settingsNew.getChatId(true) != 0 )
              debugPrintf("Last message for %lld is %lu\n", settingsNew.getChatId(), LastMsg(settingsNew.getChatId()).get() );
            // LastMsg lm(u.message().chat().id());
            // debugPrint("LastMsg:"); debugPrintln(lm.get());
          }
          break;

        case "/cat"_h:
          if ( u.message().text().count(" ") < 2 ) break;
          else {
          auto arg = u.message().text().getSub(1, " ");
          bot.setTyping(settingsNew.getAdminId(), false);
            if( ! LittleFS.exists(arg.c_str()) ){
              message.text += F("File ");
              message.text += TelegramMD::asCode( arg ); //.c_str();
              message.text += F(" not exist");
              break;
            }
            auto f = LittleFS.open(arg.c_str(), "r");
            if ( ! f ) {
              message.text += F("Error open file "); //`");
              message.text += TelegramMD::asCode( arg ); //.c_str();
              //message.text += '`';
              break; 
            } 
            String s = f.readString();
            f.close();

          // if ( settingsNew.isAdmin( u.message().from().id() ) ){ //.admin ){
          //   bot.setTyping(settingsNew.getAdminId(), false);            
          //   String s = settingsNew.loadJson();
            debugPrintln(s);
            s.replace('\\','/');
            //debugPrintln(SETTINGS::listDirToString("/"));
            //message.text;

            message.text += TelegramMD::asCode( s );
            // message.text += '`'; //TG_ATTR::code;
            // message.text += s.c_str();
            // message.text += '`'; //TG_ATTR::code;
          }
          break;
        case "/startPortal"_h:
          if (settingsNew.isAdmin( u.message().from().id() ) ){ //.admin ){
            //needStartPortal = true;
            needStart = NeedStart::Portal;
            message.text += TelegramMD::asItallic( portalStarted );
            //message.text += portalStarted_MD;
            // bot.sendMessage(message, true);
            // message.text = "";
          }
          break;
        case "/startWeb"_h:
          if (settingsNew.isAdmin( u.message().from().id() ) ){ //.admin ){
            if ( needStart == NeedStart::None ){
              needStart = NeedStart::Web;
                String txt = F("Settings [web portal](http://"); 
                txt += WiFi.localIP().toString();
                txt += F(") started...");
              message.text += TelegramMD::asItallic( txt );

              // message.text += F("_Settings [web portal](http://"); // started on _");
              // message.text += WiFi.localIP().toString();
              // message.text += F(") started \\.\\.\\._");

              if ( bot.sendMessage(message) ){
                webPortalMsgId = bot.lastBotMessage();
                message.text = "";
              }
            } 
          }
          break;
        case "/stopWeb"_h:
          if (settingsNew.isAdmin( u.message().from().id() ) ){ //.admin ){
            if ( needStart == NeedStart::WebRunning ){
              needStart = NeedStart::None;
              String closed = TelegramMD::asItallic("portal closed");
              if ( webPortalMsgId ){
                fb::TextEdit editMsg;
                editMsg.chatID = settingsNew.getAdminId();
                editMsg.text = closed; //F("_portal closed_");
                editMsg.mode = fb::Message::Mode::MarkdownV2;
                editMsg.messageID = webPortalMsgId;
                bot.editText(editMsg, false );
                webPortalMsgId = 0;
                //message.text = "";
              } else {
                message.text += closed;
              }
            }
          }
          break;
      } //switch
      if ( ! message.text.isEmpty() ) {
#if DEBUG
        runStart;
        auto res = bot.sendMessage(message);
        debugPrintln(message.text);

        auto raw = res.getRaw();
        raw.printTo(Serial);

        //send message
        printRunTime;
#else
        bot.sendMessage(message, false);
#endif
      }
    } //startsWith
};

#define QUERY_START_OPEN "~o~"
#define QUERY_TIME_START 3
#define TAKE_ADMIN "ta~"
#define DELETE_CHAT_MENU "~d~"


void updateh(fb::Update& u) {   
  if (u.isMessage() ) { 
    //wrongCount.reset();
    if( u.message().hasDocument()) handleDocument(u);
    else handleCommand(u);
  }
 
    // моё расширение для FasBot2 в chatMember.h 
  else if( fb_adds::isMyChatMember(u) ){
  //  wrongCount.reset();
    debugPretty;
    handleChatMember(u);
   }
   else if ( u.isPost() ) {
      debugPrintln("Channel post");
      if ( u.message().has( SH("new_chat_title") )){
        //debugPrintln("\n\nBingo\n\n");
        String newChatTitle = u.message().chat().title().decodeUnicode();
        long long chatId = u.message().chat().id();
        if( channelName::save( chatId, newChatTitle) ){
          fb::Message message;
          message.text = channelName::addChannelName( chatId );
          message.chatID = settingsNew.getAdminId();
          message.setModeMD();
          bot.sendMessage(message);
          debugPrintln( message.text );
         }

      } else {
        debugPrintln( u.message().text());

      }

   }
  else if (u.isQuery()) {
  //  wrongCount.reset();
    bool myAlert = false;
 
    debugPrintln("NEW QUERY");
    String txt;
    txt.reserve(100);
    long takeAdminMsgId=0;
      auto resp = u.query().data();
      debugPrint("Response '"); debugPrint(resp); debugPrintln("'");
      
      if ( resp.startsWith(QUERY_START_OPEN)) {
        auto queryChatId = u.query().message().chat().id(); // entry;
//        debugPretty; debugPrintln( queryChatId );
        if( settingsNew.getChatId(true) != 0ll && 
            queryChatId != settingsNew.getChatId(true) ){
          txt += CHANNEL_FOR_CONTROL;
          txt += F("'");
          channelName::load(settingsNew.getChatId(true));
          txt += channelName::get();
          txt += F("'");
          myAlert = true;
        } else {
          // проверяем время на кнопке
/*           time_t _now = time(nullptr);
          time_t buttonTime = resp.substring(QUERY_TIME_START).toInt32();
          debugPrintf("Button exptime=%ld now=%ld", buttonTime + ( POLLING_TIME / ( 1 SEC ) ) + 1, _now);
          if ( _now - buttonTime <= ( POLLING_TIME / ( 1 SEC ) ) + 1 ){ */
          long buttonTime = resp.substring(QUERY_TIME_START).toInt32();
          if ( ! myButton.isExpired( buttonTime, 3000 ) ){
            relay.open();
            txt = settingsNew.getButtonReport(); //settings.chat.button.report;  
            //getNameFromMessage(txt, u, (char *)F(", ") );
            getNameFromRead(txt, u.message().from(), (char *)F(", ") ); 
          } else {  
            //debugPrintf("")
            txt = TRY_LATTER;
            myAlert = true;
          }
        }

// стать администратором          
      } else if( resp.startsWith(TAKE_ADMIN) ){
        takeAdminMsgId = resp.substring(QUERY_TIME_START).toInt32();
        myAlert=true;  
        if( settingsNew.getAdminId() ){
          
          txt += haveAdmin_Alert; // F("У меня уже есть хозяин!");
          txt += this_bot_link;
          
        } else {

          settingsNew.set()->AdminId( u.message().from().id() );
          if ( settingsNew.save() ){
            
            myButton.creater( settingsNew.getAdminId(), settingsNew.getButton() );
            getNameFromRead(txt, u.message().from(), (char *)F("Поздравляю! "), (char *)F(", теперь я твой раб.") );
            //myButton.needUpdate();
            //

          } else {
            txt += BotSettings::NotSaveStr; // F("Упс. Что то с памятью моей стало.\nНе могу записать настройки.");
          }
        }

      } else if( resp.startsWith( "up" )) {
        // do GitHub upgrade
        GitHubUpgrade::needUpgrade = true;
        
      }


    bot.answerCallbackQuery(u.query().id(), txt.c_str(), myAlert, false );
    channelName::freeMemory();

    if( takeAdminMsgId ){ //} && u.message().from().id() == takeAdmin.userId ){
     /* fb::Result res = */ bot.deleteMessage(u.message().from().id(), takeAdminMsgId, false); //takeAdmin.userId, takeAdmin.msgId);
    }
  } 
}

