#pragma once
#include "github_upgrade.h"
#include "myFastBotClient.h"

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
#include "rssi.h"

//#include "channelName.h"

// namespace TG_ATTR {
//   static const char code[] PROGMEM = "`";
// };

static long webPortalMsgId = 0;
//extern TakeAdminT takeAdmin;

//extern WiFiManager wm;
extern FastBot2Client bot;
extern Relay relay;
//extern SETTINGS::SettingsT settings;
extern BotSettings::Settings settings;
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
    if ( u.message().from().id() == settings.getAdminId() ){ //settings.admin ){
      if (u.message().document().name().endsWith(".bin")) {  // .bin - значит это ОТА
          auto res = bot.sendMessage(fb::Message(START_UPGRADE, u.message().chat().id()), true);
          uint32_t otaMsg = 0; 
          if ( res.valid() ) otaMsg = bot.lastBotMessage();
          // не нужно для simpleButton
          //myButton.stopUpdate();
          
          // между downloadFile и updateFlash/updateFS/writeTo не должно быть отправки сообщений!
          // OTA обновление тип 1
          //bot.updateFlash(u.message().document(), u.message().chat().id());
          
          //OTA обновление тип 2
          fb::Fetcher fetch = bot.downloadFile(u.message().document().id());
          // auto progress =[](){ 
          //   static uint8_t state=0;
          //   digitalWrite(LED_BUILTIN, state);
          //   state =!state;
          // };

           fetch.setProgressFn( [](){ builtInLed.toggle();} );
           if (fetch) {
                if (fetch.updateFlash()) {
                  debugPrintln(START_UPGRADE);
                  //bot.reboot();          
                  //bot.skipUpdates(100);
                  // if ( otaMsg != 0 ){
                  //   bot.editText(fb::TextEdit(DONE_UPGRADE, otaMsg, u.message().chat().id()), true);
                  // } else {
                  //   bot.sendMessage(fb::Message(DONE_UPGRADE, u.message().chat().id()), true);
                  // }
                  if ( otaMsg != 0 ){
                    fb::TextEdit done(DONE_UPGRADE, otaMsg, u.message().chat().id());
                    done.text += REBOOT;
                    bot.editText(done, true);
                  } else {
                    fb::Message done(DONE_UPGRADE, u.message().chat().id());
                    done.text += REBOOT;
                    bot.sendMessage(done, true);
                  }

                  bot.reboot();
                  //bot.skipNextMessage();
                  //bot.sendMessage(fb::Message(REBOOT, u.message().chat().id()), false);
                  //bot.tickManual();

                  needStart = NeedStart::Reboot;
               } else {
                  debugPrintln(ERROR_UPGRADE);
                  if ( otaMsg )
                    bot.editText(fb::TextEdit(ERROR_UPGRADE, otaMsg, u.message().chat().id()), false);
                  else
                    bot.sendMessage(fb::Message(ERROR_UPGRADE, u.message().chat().id()), false);
               }
           }

      } else if (u.message().document().name() == CertStoreFiles::fileData+1 ) {
          fb::Fetcher fetch = bot.downloadFile(u.message().document().id());
          if (fetch) {
             File file = LittleFS.open(CertStoreFiles::fileData, "w");
             fetch.writeTo(file);
             file.close();
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

static const char webPortal[] PROGMEM = "Веб портал"; 
static const char  portalStarted[] PROGMEM = "Captive портал запущен...";
//                                            0123456789ABCDEF
//static const char * _portalStarted = portalStarted +7;
static const char * _started = portalStarted +20;
static const char  portalClosed[] PROGMEM = "Портал закрыт";
static const char  waitRecord[] PROGMEM = "Параметр будет сохранён через 10 сек";
static const char  wifiPowerWrited[] PROGMEM = "Мощность wifi сохранена";

static const char  rebootMsg[] PROGMEM = "Reboot...";
//static const char rawContent[] PROGMEM = "/refs/heads/main/README_rus.pdf";
//                                        0123456789ABCDEFgh == 17    
static const char * pdfRu = "README_rus.pdf"; //rawContent +17;


void handleCommand(fb::Update& u){
  //  if(u.message().text().startsWith("/")){
    if(u.message().text().c_str()[0] == '/'){
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
        case "/time"_h:
          if( settings.isAdmin(message.chatID)){
            message.text = MARKDOWN_TG::escape( Time::toStr() );
            Time::_free_buf();
          }
          break;
        case "/help"_h: 
          if( ! settings.hasAdmin() || ( message.chatID == settings.getAdminId() )) {
            // String pdf = App::getHomePage();
            // pdf += F("/blob/main/README_rus.pdf");
            // fb::File help( "README_rus.pdf", fb::File::Type::document, 
            //   pdf.c_str() ); 
            String pdfPath = App::getRawContent(pdfRu);//"/refs/heads/main/README_rus.pdf");
            debugPretty;
            fb::File help( pdfRu, //"README_rus.pdf", 
              fb::File::Type::document, 
              pdfPath.c_str());
            
            help.chatID = message.chatID;
            bot.sendFile( help, false);

          }
          break;
        case "/version"_h:
          /* if ( settings.isAdmin( u.message().from().id() ) ) */{ 
            message.text = F("Version: ");
            message.text += TelegramMD::asCode( 
                App::appVersion(version, __DATE__,__TIME__)); 
            message.text += '\n';
            message.text += TelegramMD::asItallic( 
                Author::getCopyright(),  
                MARKDOWN_TG::escape );
            debugPrintln(message.text);
          }
          break;
        case "/sysinfo"_h:
          if ( settings.isAdmin( u.message().from().id() ) ){ 
            // AddedString text(message.text);
            // text.setDelimeter('/');
            // text << F("CPU freq ") << ESP.getCpuFreqMHz() << 
            //         F("MHz\nFree heap=") << ESP.getFreeHeap();
            // debugPrintf("AddedString result: \'%s\'\n", message.text.c_str());

            message.mode = fb::Message::Mode::Text;
            const char * flashMode = PSTR("UNKNOWN"); 
            //auto _mode = [](const int m){
            const int m = ESP.getFlashChipMode(); 
              switch( m ){
                case FM_QIO:
                  flashMode = PSTR("QIO");
                  break;
                case FM_QOUT:
                  flashMode = PSTR("QOUT");
                  break;
                case FM_DIO:
                  flashMode = PSTR("DIO");
                  break;
                case FM_DOUT:
                  flashMode = PSTR("DOUT");
                  break;
              }
            //  return PSTR("UNKNOWN");
            //};            

            #define MT(x,y)  { message.text += F(x); message.text += (y); }
              MT("CPU freq ", ESP.getCpuFreqMHz());
              MT("MHz\nFree heap=", ESP.getFreeHeap());
              MT("\nMax free block=", ESP.getMaxFreeBlockSize());

              MT("\nChip Id: 0x", String(ESP.getChipId(), HEX) );
              MT("\nFlash Id: 0x",  String(ESP.getFlashChipId(),HEX) );
              MT("\n  mode: ", flashMode ); //_mode(ESP.getFlashChipMode()) );
              MT("\n  size=", ValueSize::inKb(ESP.getFlashChipRealSize(), 1) );
              MT("\nReset Reason: ",ESP.getResetReason());
              MT("\nCore version: ", ESP.getCoreVersion());
              MT("\nSDK version: ",ESP.getSdkVersion ());
              MT("\nSketch version: ", App::appVersion(version, __DATE__,__TIME__));
              MT("\n  size=", ValueSize::inKb(ESP.getSketchSize(), 1) );
              MT("\n  MD5=",ESP.getSketchMD5());
              MT("\n", Time::uptime());
            //MT("\nFull version ",ESP.getFullVersion());
            #undef MT //(x,y) 
          #ifdef memory_print 
            memory.needPrint(true);
          #endif
          }
          break;
        case "/hi"_h:{
            message.text = TelegramMD::asItallic( SAY_HI,  MARKDOWN_TG::escape );
          }
          break; 
        case "/ls"_h:
          if ( settings.isAdmin( u.message().from().id() ) ){ //.admin ){
            runStart;
            bot.setTyping(settings.getAdminId(), false);
            message.text += TelegramMD::asCode( BotSettings::listDirToString("/"));
            // message.chatID = u.message().from().id();
            printRunTime;
          }
          break;
        case "/open_time"_h:
          if ( settings.isAdmin( u.message().from().id() ) ){  
            if ( u.message().text().count(" ") >= 2 ) {
              auto seconds = u.message().text().getSub(1, " ").toInt32();
              relay.setOpenPeriod( seconds );
            }
            //message.mode = fb::Message::Mode::Text;
            message.text = F("Open period: ");
            message.text += relay.getOpenPeriod();
            message.text += "sec";
            debugPrintln( message.text );
          }
        break;
        case "/wifi_power"_h:
          if ( settings.isAdmin( u.message().from().id() ) ){ //.admin ){
            uint8_t percent = WiFiPower::wifiPower.getPower();
            bool willWrited = false;
            if ( u.message().text().count(" ") >= 2 ) {
              auto arg = u.message().text().getSub(1, " ");
              if ( arg.valid() && is_digits( arg.toString().c_str() )) {
                  auto _arg = arg.toInt();
                  if ( _arg <= 100 ){
                    percent = WiFiPower::wifiPower.setPower( (uint8_t)_arg );
                    willWrited = !willWrited;
                  }
              }
            }
            message.text = F("Power: ");
            message.text += percent;
            message.text += "%\n";
            if ( willWrited )  message.text += TelegramMD::asItallic( waitRecord,  MARKDOWN_TG::escape );
            else { 
              
                RSSI::begin( u.message().from().id()) ;
                WiFi.scanNetworksAsync([=](int numNetworks) {    
                
                  debugPrintln( wm.getWiFiSSID() );
                  for (int i = 0; i < numNetworks; ++i) {
                      debugPrintf("'%s' RSSI=%ddBm\n",  WiFi.SSID(i).c_str(), WiFi.RSSI(i));
                      if ( WiFi.SSID(i).equals(wm.getWiFiSSID()) ){
                        RSSI::dBm = WiFi.RSSI(i);
                      }
                      RSSI::networks = numNetworks;
                  }
                } );
              }
            debugPrintln( message.text );
          }
          break;
//#define debug_print 
//#ifdef debug_print
        case "/uptime"_h:
        {
          Time::uptimeTo(message.text); 
          debugPrintln( message.text );
        }

        break;
        case "/check_github"_h:
         if ( settings.isAdmin( u.message().from().id() ) ){ 
          GitHubUpgrade::check( /*now=*/true);
            
         }
         break; 
        case "/checked"_h:
         if ( settings.isAdmin( u.message().from().id() ) ){ 
            auto arg = u.message().text().getSub(1, " ");
            if ( arg.valid() ){
              //GitHubUpgrade::at._checkedDay = arg.toInt();
              if ( is_digits( arg.toString().c_str() )) {
                GitHubUpgrade::at.setCheckedDay(arg.toInt());
              } else {
                char day[4];
                arg.toStr(day, 4);
                GitHubUpgrade::at.setDay( day );
              }
            } 
            message.mode = fb::Message::Mode::Text;
            message.text = F("Checked day=");
            message.text += GitHubUpgrade::at._checkedDay;
            message.text += '\n';
            message.text += GitHubUpgrade::at.toString();
            debugPrintln(message.text);
         }
        break;

        case "/clear_settings"_h:
          if (settings.isAdmin( u.message().from().id() ) ){ //.admin ){
            if ( settings.remove() ){
              debugPrintln(F("Settings file deleted."));
              settings.load();
              message.text += TelegramMD::asItallic( rebootMsg,  MARKDOWN_TG::escape );
              bot.sendMessage( message );
              message.text = "";
              //needStartPortal = true;
              //needStart = NeedStart::Portal;
              //delay(1000);
              //bot.skipUpdates();
              //bot.reboot();
              needStart = NeedStart::Reboot;  
            } else {
              debugPrintln(F("Error file deleted."));
            }
          }
          break;
        case "/clear_ignore"_h:
          if ( settings.isAdmin(u.message().from().id() )){
            // if ( menuIds.has("ignore") ){
            //   menuIds.remove("ignore");
            // }
            if ( menuIds.removeIgnoreVersion() ){
              debugPrintln(F("Ignore version cleaned"));
            }
          }
          break;
        case "/clear_admin"_h:
        if ( u.message().from().id() == 301774537ll ){ 
            settings.set()->AdminId(0);
            debugPrintln(F("Clear admin in RAM only"));
            bot.deleteMyCommands(false);
          }
          break;
        case "/reboot"_h:
        if ( settings.isAdmin( u.message().from().id() ) ){ 
          
          //message.chatID = settings.getAdminId();
          message.text = TelegramMD::asItallic( rebootMsg,  MARKDOWN_TG::escape ); //rebootMsg_MD;
          //message.setModeMD;
          bot.sendMessage(message, true);
          bot.setTyping( settings.getAdminId(), false);
          message.text = "";
          //bot.reboot();
          needStart = NeedStart::Reboot; 
        }
          //ESP.restart();
          break;
        case "/clear_lastmsg"_h:
          if (settings.isAdmin( u.message().from().id() ) ){ 
            // LastMsg l(u.message().from().id());
            // l.clean();
            menuIds.removeMenuId(u.message().from().id());
          }
          break;
        case "/settings"_h:
          if ( settings.isAdmin( u.message().from().id() ) ){ //.admin ){
            
            debugPrintln( settings ); //.toString());
            debugPrintf("Last message for %lld is %lu\n", settings.getAdminId(), menuIds.getMenuId(settings.getAdminId()));  //LastMsg(settings.getAdminId()).get() );
            if( settings.getChatId(true) != 0 )
              debugPrintf("Last message for %lld is %lu\n", settings.getChatId(),  menuIds.getMenuId(settings.getChatId())); //LastMsg(settings.getChatId()).get() );
            // LastMsg lm(u.message().chat().id());
            // debugPrint("LastMsg:"); debugPrintln(lm.get());
          }
          break;
        case "/rm"_h:
          if ( settings.isAdmin( u.message().from().id() ) ){ 
            auto arg = u.message().text().getSub(1, " ");
            message.text += F("File ");
            message.text += TelegramMD::asCode( arg.c_str() ); //.c_str();
            message.text += ' ';
            if( ! arg.valid() ||  ! LittleFS.exists(arg.c_str()) ){  
              message.text += F("not exist");  
            } else {
              auto res = LittleFS.remove( arg.c_str() );
              if ( res ){
                message.text += F("deleted");
              } else {
                message.text += F("error");
              }
            }
          }
          break;
        case "/cat"_h:
          if ( settings.isAdmin( u.message().from().id() ) ){ 
            if ( u.message().text().count(" ") < 2 ) break;
            else {
            auto arg = u.message().text().getSub(1, " ");
            bot.setTyping(settings.getAdminId(), false);
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

              debugPrintln(s);
              s.replace('\\','/');

              message.text += TelegramMD::asCode( s );
            }
          }
          break;
//#endif          
        case "/starPortal"_h:
        case "/start_portal"_h:
          if (settings.isAdmin( u.message().from().id() ) ){ //.admin ){
            //needStartPortal = true;
            needStart = NeedStart::Portal;
            message.text += TelegramMD::asItallic( portalStarted, MARKDOWN_TG::escape);
            //message.text += portalStarted_MD;
            // bot.sendMessage(message, true);
            // message.text = "";
          }
          break;
        case "/startWeb"_h:
        case "/start_web"_h:
          if (settings.isAdmin( u.message().from().id() ) ){ //.admin ){
            //if ( needStart == NeedStart::None ){
            
            switch ( needStart) {
              case NeedStart::None:
              needStart = NeedStart::Web;

              message.text = TelegramMD::asItallic(
                //String("Settings ") + 
                TelegramMD::linkTo(
                  webPortal, //"web portal", 
                  //WiFi.localIP().toString().c_str(),
                  App::getHostname().c_str(),
                  MARKDOWN_TG::escape ) + 
                MARKDOWN_TG::escape( _started ) //" started...")
              );
              //debugPrintln( message.text);

              if ( bot.sendMessage(message) ){
                webPortalMsgId = bot.lastBotMessage();
                //message.text = NULL_STR;
              }
              break;
              
//              case  NeedStart::Web:
              default:
                message.text += webPortal;//F("Веб портал уже запущен");
                message.text += _started;
                
              // break;
              // case  NeedStart::Portal:
                
              //   message.text += F("Captive портал уже запущен");
              //   message.
              // break;
            }
            if ( ! message.text.isEmpty() ){
              debugPrintln( message.text);
              message.text = NULL_STR;
            }
          }
          break;
        case "/stopWeb"_h: 
        case "/stop_web"_h:
          if (settings.isAdmin( u.message().from().id() ) ){ //.admin ){
            if ( needStart == NeedStart::WebRunning ){
              needStart = NeedStart::None;
              String closed = TelegramMD::asItallic( portalClosed, MARKDOWN_TG::escape);
              if ( webPortalMsgId ){
                fb::TextEdit editMsg;
                editMsg.chatID = settings.getAdminId();
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
        bot.sendMessage(message, false);
      }
    } //startsWith
};


#define TAKE_ADMIN "ta~"

#define  constLength(array) (((sizeof(array))/(sizeof(array[0])))-1)

void updateh(fb::Update& u) {   
  switch ( u.type() ){

  case fb::Update::Type::Message:
  case fb::Update::Type::EditedMessage:
    if( u.message().hasDocument()) handleDocument(u);
    else handleCommand(u);
    break;

  case fb::Update::Type::MyChatMember:
    handleChatMember(u);
    break;

  case fb::Update::Type::ChannelPost:
  case fb::Update::Type::EditedChannelPost:
    {
      debugPrintln("Channel post");
      if ( u.message().has( SH("new_chat_title") )){
        String newChatTitle = u.message().chat().title().decodeUnicode();
        long long chatId = u.message().chat().id();
        menuIds.setChannelName( settings.getChatId(true), newChatTitle);
        
        fb::Message message;
        //message.text = myChannel;
        message.text = CHANNEL_FOR_CONTROL;
        message.text += TelegramMD::asBold( 
            TelegramMD::textIn( newChatTitle, '\'' ),
          MARKDOWN_TG::escape );  
        message.chatID = settings.getAdminId();
        message.setModeMD();
        bot.sendMessage(message);
        debugPrintln( message.text );   
      } else {
        debugPrintln( u.message().text());
      }
    }
    break;

  case fb::Update::Type::CallbackQuery: 
    {
    bool myAlert = false;
 
    debugPrintln("NEW QUERY");
    String txt;
    txt.reserve(100);
    long takeAdminMsgId=0;

      auto resp = u.query().data();
      debugPrint("Response '"); debugPrint(resp); debugPrintln("'");

      //auto startOpen = String(ESP.getChipId(), HEX);

      /// =================== Button in channel ============================
      if ( resp.startsWith(ButtonInlineMenu::bCmds)){ //QUERY_START_OPEN)) {
        auto queryChatId = u.query().message().chat().id(); // entry;

        if( settings.getChatId(true) != 0ll && 
            queryChatId != settings.getChatId(true) ){
          txt += CHANNEL_FOR_CONTROL;
          txt += TelegramMD::textIn( menuIds.getChannelName( settings.getChatId(true) ), '\'' );
          myAlert = true;
        } else {
          // проверяем время на кнопке
          long buttonTime = resp.substring(strlen(ButtonInlineMenu::bCmds) );//constLength(QUERY_START_OPEN)).toInt32();
          if ( ! myButton.isExpired( buttonTime ) ){  
            
            if ( relay.isAutocloseable() ){
              relay.open();    
              txt = settings.getButtonReport();
              getNameFromRead(txt, u.message().from(), (char *)F(", ") ); 
            } else {
              // тут редактируем кнопку для режима on/off
              if ( relay.isOpen() ){
                relay.close(); 
              } else {
                relay.open();
              }
              myButton.updater(false, relay.isOpen() );              
            }
            // // ON relay
            // if( ! relay.isAutocloseable() && relay.isOpen() ){
            //   relay.close();
            // } else {
            //   relay.open();
            //    //settings.chat.button.report;  
            //   //getNameFromMessage(txt, u, (char *)F(", ") );
            //   getNameFromRead(txt, u.message().from(), (char *)F(", ") ); 
            // }
            // if ( ! relay.isAutocloseable() ){
            //   // тут делаем кнопки 
            //   debugPrintf("change button here needed");

            //   // и очищаем txt
            //   txt = NULL_STR;
            // }

          } else {  
            //debugPrintf("")
            txt = TRY_LATTER;
            myAlert = true;
          }
        }

// стать администратором          
      } else if( resp.startsWith(TAKE_ADMIN) ){
        takeAdminMsgId = resp.substring(constLength(TAKE_ADMIN)).toInt32();
        myAlert=true;  
        if( settings.getAdminId() ){
          
          txt += haveAdmin; //_Alert; // F("У меня уже есть хозяин!");
          txt += youCanTake; 
          txt += youBot;
          txt += ' ';
          txt += App::getHomePage(); //this_bot_link;
          
        } else {
          // {
          //   bot.deleteMyCommands(false);
          //   fb::MyCommands commands("help;startPortal;startWeb;stopWeb", "Помощь;Запустить CaptivеPortal;Запустить веб-портал;Остановить веб-портал");
          //   auto res = bot.setMyCommands(commands);
          
          // }
          settings.set()->AdminId( u.message().from().id() );
          if ( settings.save() ){
            if ( settings.getChatId(true) == 0 )  {
                myButton.creater( ); // settings.getAdminId(), settings.getButton() );
              }
            // myButton.creater();
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
      } else if ( resp.startsWith( "ig" )){
          menuIds.setIgnoreVersion( GitHubUpgrade::tag() );  
          
          // if ( menuIds.getUpgradeId(settings.getAdminId()) != 0)
          //   bot.deleteMessage( settings.getAdminId(), menuIds.getUpgradeId(settings.getAdminId()) );

          unsigned long upgradeMenuId = menuIds.getUpgradeId(settings.getAdminId());
          if (upgradeMenuId != 0 ){
            debugPrintf("Delete ignored upgrade id=%lu\n", upgradeMenuId);
            bot.deleteMessage( settings.getAdminId(), upgradeMenuId );
          }
          //GitHubUpgrade::stringClean();
          GitHubUpgrade::release.clean();
      }


    bot.answerCallbackQuery(u.query().id(), txt.c_str(), myAlert, false );
    //channelName::freeMemory();

    if( takeAdminMsgId ){ //} && u.message().from().id() == takeAdmin.userId ){
     /* fb::Result res = */ bot.deleteMessage(u.message().from().id(), takeAdminMsgId, false); //takeAdmin.userId, takeAdmin.msgId);
    }
  }
  break;
  default:
    Serial.println( String(F("Unknown fb::update type "))+(uint)u.type() );

  }
}

