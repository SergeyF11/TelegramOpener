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

#include "report.h"
#include "backup.h"


// static const char  portalStarted_MD[] PROGMEM = "_Config portal started\\.\\.\\._";
// static const char  rebootMsg_MD[] PROGMEM = "_Reboot\\.\\.\\._";

static const char webPortal[] PROGMEM = "Веб портал"; 
static const char  portalStarted[] PROGMEM = "Captive портал запущен...";

static const char * _started = portalStarted +20;
static const char  portalClosed[] PROGMEM = "Портал закрыт";
static const char  waitRecord[] PROGMEM = "Параметр будет сохранён через 10 сек";
static const char  wifiPowerWrited[] PROGMEM = "Мощность wifi сохранена";

//static const char  rebootMsg[] PROGMEM = "Reboot...";
//static const char rawContent[] PROGMEM = "/refs/heads/main/README_rus.pdf";
//                                        0123456789ABCDEFgh == 17    
static const char * pdfRu = "README_rus.pdf"; //rawContent +17;

extern String botName;

//#include "channelName.h"

// namespace TG_ATTR {
//   static const char code[] PROGMEM = "`";
// };

static long webPortalMsgId = 0;
//extern TakeAdminT takeAdmin;

//extern WiFiManager wm;
extern FastBot2Client bot;
extern Relay relay;
extern bool isRelayOn();
//extern SETTINGS::SettingsT settings;
extern BotSettings::Settings settings;
//extern bool needStartPortal;
extern NeedStartE needStart;
//extern LastMsg lastMsg;
extern bool mdnsStarted;

//time_t loopNow;


bool getNameFromRead(String& txt, fb::UserRead ur, const char* prefix=((char *) 0), const char* postfix=((char *)0) ){
  bool hidden = false;

  txt += prefix; //F("Поздравляю! "); 
  
  if ( ur.firstName().length() != 0 ) txt += ur.firstName().decodeUnicode();
  else if ( ur.username().length() != 0 ) txt += ur.username().toString() ;
  else { 
    hidden = true;
    txt += F("Незнакомец с id#");
    txt += ur.id().toString();
  }
  txt += postfix; //F(", теперь я твой раб.");
  return !hidden;
};

bool  getNameFromRead(String& txt,  fb::UserRead ur, const String& prefix, const String& postfix ){
  return getNameFromRead(txt, ur, prefix.c_str(), postfix.c_str());
};

void versionTo( fb::Message& message ){ 
  message.text = F("Version: ");
  message.text += TelegramMD::asCode( 
      App::appVersion(version, __DATE__,__TIME__)); 
  message.text += '\n';
  message.text += TelegramMD::asItallic( 
      Author::getCopyright(),  
      MARKDOWN_TG::escape );
  debugPrintln(message.text);
};

bool sendHelp( fb::Message& message ){

    String pdfPath = App::getRawContent(pdfRu);//"/refs/heads/main/README_rus.pdf");
    
    debugPrintf("Send document link: %s\n", pdfPath.c_str());
    fb::File help( (Text)pdfRu, //"README_rus.pdf", 
        fb::File::Type::document, 
        pdfPath.c_str());

    
    help.chatID = message.chatID;
    auto res = bot.sendFile( help, true); //false);

    debugPrint("Resp: "); 
    #ifdef debug_print 
    res.printTo(Serial);
    #endif

    if ( res.valid() && !res.isError() ) 
      return true;
    
    // else
    message.text = F("Ошибка получения файла! Поробуйте скачать:");
    message.text += TelegramMD::asCode( pdfPath );
    return false;               
};

void sysinfoTo(String& out)
{
    static char sketchMD5[33] = { 0 };
    if ( sketchMD5[0] == 0  ) {
      auto md5Str = ESP.getSketchMD5();
      yield();
      strncpy(sketchMD5, md5Str.c_str(), sizeof(sketchMD5) - 1);
      sketchMD5[sizeof(sketchMD5) - 1] = '\0';
    }
    //static const String sketchMD5( ESP.getSketchMD5() );
    
    static uint32_t realSize = 0;
    if ( ! realSize ) realSize = ESP.getFlashChipRealSize();
    
    
    char buffer[512];
    int offset = 0;
    
    const uint32_t freeHeap = ESP.getFreeHeap();
    const uint32_t maxFree  = ESP.getMaxFreeBlockSize();
  

    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
        "CPU freq %uMHz\nFree heap=%u\nMax free block=%u\n",
        ESP.getCpuFreqMHz(), freeHeap, maxFree );
    
    yield();
    debugPrintln( buffer );

    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
        "Chip Id: 0x%08X\nFlash Id: 0x%08X\n",
        ESP.getChipId(), ESP.getFlashChipId());
    
    const char* flashMode = [](){
        switch(ESP.getFlashChipMode()) {
            case FM_QIO:  return PSTR("QIO");
            case FM_QOUT: return PSTR("QOUT");
            case FM_DIO:  return PSTR("DIO");
            case FM_DOUT: return PSTR("DOUT");
            default:      return PSTR("UNKNOWN");
        }
    }();
      
    debugPrintln( buffer );
    
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
        "  mode: %s\n  size=%s\n",
        flashMode, ValueSize::inKb(realSize, 1).c_str());
    
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
        "Reset Reason: %s\nCore version: %s\nSDK version: %s\n",
        ESP.getResetReason().c_str(),
        ESP.getCoreVersion().c_str(),
        ESP.getSdkVersion());
    
    yield();
    debugPrintln( buffer );
    
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
        "Sketch version: %s\n  size=%s\n",
        App::appVersion(version, __DATE__, __TIME__).c_str(),
        ValueSize::inKb(ESP.getSketchSize(), 1).c_str());
    
    // MD5 вычисление - может быть долгим
    offset += snprintf(buffer + offset, sizeof(buffer) - offset,
        "  MD5=%s\n%s\n",
        sketchMD5,
        Time::uptime().c_str());
    
    yield();
    debugPrintln( buffer );
    
    out = buffer;
    
}

// void sysinfoTo( fb::Message& message )                
// { 
//     // AddedString text(message.text);
//     // text.setDelimeter('/');
//     // text << F("CPU freq ") << ESP.getCpuFreqMHz() << 
//     //         F("MHz\nFree heap=") << ESP.getFreeHeap();
//     // debugPrintf("AddedString result: \'%s\'\n", message.text.c_str());

//     message.mode = fb::Message::Mode::Text;
//     const char * flashMode = PSTR("UNKNOWN"); 
//     //auto _mode = [](const int m){
//     const int m = ESP.getFlashChipMode(); 
//       switch( m ){
//         case FM_QIO:
//           flashMode = PSTR("QIO");
//           break;
//         case FM_QOUT:
//           flashMode = PSTR("QOUT");
//           break;
//         case FM_DIO:
//           flashMode = PSTR("DIO");
//           break;
//         case FM_DOUT:
//           flashMode = PSTR("DOUT");
//           break;
//       }
//     //  return PSTR("UNKNOWN");
//     //};            

//     #define MT(x,y)  { message.text += F(x); message.text += (y); }
//       MT("CPU freq ", ESP.getCpuFreqMHz());
//       MT("MHz\nFree heap=", ESP.getFreeHeap());
//       MT("\nMax free block=", ESP.getMaxFreeBlockSize());

//       MT("\nChip Id: 0x", String(ESP.getChipId(), HEX) );
//       MT("\nFlash Id: 0x",  String(ESP.getFlashChipId(),HEX) );
//       MT("\n  mode: ", flashMode ); //_mode(ESP.getFlashChipMode()) );
//       MT("\n  size=", ValueSize::inKb(ESP.getFlashChipRealSize(), 1) );
//       MT("\nReset Reason: ",ESP.getResetReason());
//       MT("\nCore version: ", ESP.getCoreVersion());
//       MT("\nSDK version: ",ESP.getSdkVersion ());
//       MT("\nSketch version: ", App::appVersion(version, __DATE__,__TIME__));
//       MT("\n  size=", ValueSize::inKb(ESP.getSketchSize(), 1) );
//       MT("\n  MD5=",ESP.getSketchMD5());
//       MT("\n", Time::uptime());
//     //MT("\nFull version ",ESP.getFullVersion());
//     #undef MT //(x,y) 
//   #ifdef memory_print 
//     memory.needPrint(true);
//   #endif
//   }


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
    if ( settings.isAdmin( u.message().from().id() ) ){ //settings.admin ){


      auto docId = u.message().document().id();
      auto docName = u.message().document().name();
      int32_t fromId = u.message().from().id().toInt32();
      //int64_t _chatId = u.message().chat().id().toInt64();
      
      if ( docName.endsWith(".bin")) {  // .bin - значит это ОТА 
          fb::Message msg(START_UPGRADE, fromId);
          msg.mode = fb::Message::Mode::MarkdownV2;
          auto res = bot.sendMessage( msg, true);
          const uint32_t otaMsg = ( res.valid() && ! res.isError() ) ? bot.lastBotMessage() : 0;

          debugPrintln(START_UPGRADE);
          //OTA обновление тип 2
          fb::Fetcher fetch = bot.downloadFile( docId );

           if (fetch) {
                if (fetch.updateFlash()) {
                  debugPrintln(DONE_UPGRADE);
 
                  msg.text = DONE_UPGRADE;
                  msg.text += REBOOT_MD;

                  if ( otaMsg != 0 ){
                    fb::TextEdit done(msg.text, otaMsg, fromId );
                    //done.text += REBOOT;
                    done.mode = fb::Message::Mode::MarkdownV2;
                    bot.editText(done, true);
                  } else {
                    // fb::Message done(DONE_UPGRADE, u.message().chat().id());
                    // done.text += REBOOT;
                    // done.mode = fb::Message::Mode::MarkdownV2;
                    bot.sendMessage(msg, true);
                  }

                  bot.reboot();
                  needStart = NeedStartE::Reboot;
                  //msg.text = "";

               } else {
                  debugPrintln(ERROR_UPGRADE);

                  msg.text = ERROR_UPGRADE;

                  if ( otaMsg ){
                    fb::TextEdit msg(ERROR_UPGRADE, otaMsg, fromId );
                    msg.mode = fb::Message::Mode::MarkdownV2;
                    bot.editText( msg, false);
                  } else {
                    // fb::Message msg(ERROR_UPGRADE, u.message().chat().id());
                    // msg.mode = fb::Message::Mode::MarkdownV2;
                    bot.sendMessage( msg, false);
                  }
                }
           }

      } else if (docName == CertStoreFiles::fileData+1 ) {
          fb::Message msg(RENEW_CERTSSTORE_MD, fromId );
          msg.mode = fb::Message::Mode::MarkdownV2;
          auto res = bot.sendMessage( msg, true);
          uint32_t msgId = 0;
          if ( res.valid() && !res.isError() )
            msgId = bot.lastBotMessage();


          fb::Fetcher fetch = bot.downloadFile( docId );
          if (fetch) {
            File file = LittleFS.open(CertStoreFiles::fileData, "w");
            fetch.writeTo(file);
            file.close();
            msg.text = CERTSSTORE_LOADED_MD;//F("_Новый пакет сертификатов загружен_ ");
            msg.text += NEED_REBOOT_MD;
            msg.text += REBOOT_MD;

            debugPrintln( msg.text );

            if ( msgId != 0){
              fb::TextEdit _msg( msg.text, msgId, fromId );
              _msg.mode = fb::Message::Mode::MarkdownV2;              
              auto res = bot.editText( _msg, true );
              // delay(200);
              debugPrintf( "Edit msg[%u] res=", msgId );
              res.printTo(Serial);

            } else {
              auto res = bot.sendMessage(msg, true );
              debugPrint( "Send msg res=" );
              res.printTo(Serial);
            }
            bot.reboot();
            needStart = NeedStartE::Reboot;

          } else {
            msg.text = F("_Ошибка загрузки_");
            if ( msgId != 0){
              fb::TextEdit msg( msg.text, msgId, fromId );
              msg.mode = fb::Message::Mode::MarkdownV2;              
              bot.editText( msg, true );
              // delay(200);
            }
          }
      } else if ( docName.endsWith(".ar")) {
          if ( ! Backup::restore() ){
            fb::Message msg("TODO: release backup function", fromId );
            bot.sendMessage(msg );
          }
      
      } else {
        String unknownFile = F("Unknown file: ");
        unknownFile += docName.toString();
        //bot.answerCallbackQuery(u.query().id(), unknownFile.c_str());
        debugPrintln(unknownFile);
      }
    }
};




void setReaction(const int64_t chatId, const int32_t msgId, const char * emoji, bool wait = true) {
  // Формируем JSON вручную для передачи массива реакций
  static constexpr const char tmpl[] PROGMEM = 
    "{\"chat_id\":\"%lld\","
    "\"message_id\":%ld,"
    "\"reaction\":["
      "{\"type\":\"emoji\",\"emoji\":\"%s\"}"
    "]}";
  char payload[128];
  auto len = snprintf( payload, sizeof(payload), tmpl,
    chatId, msgId, emoji );

  if( len > 0) {
    //payload[len] = '\0';
    debugPrintln( payload );
  
    // Отправляем через встроенный метод sendCommand
    auto res = bot.sendCommand( tg_cmd::setMessageReaction, payload, wait);
    if ( wait ) {
          if( res.valid() && ! res.isError() ){
            debugPrintln( "ok:" );
            debugPrintln( res.toString() );
          } else {
            debugPrintln( "Error:" );
            debugPrintln( res.toString() );
          }
    }
  }
}

inline void setReaction( fb::MessageRead msg, const char * emoji) {
  setReaction( msg.chat().id(), msg.id(), emoji);
}

bool checkGroupChat(fb::Update& u ){
  debugPrintf("Type=%u, msg:'%s'\n", 
    (size_t)u.message().chat().type(),
    u.message().text().decodeUnicode().c_str()
  );

  debugPrintf( "ChatId=%s, MsgId=%s\n", 
    u.message().chat().id().toString().c_str(),
    u.message().id().toString().c_str()
  );
  

  switch ( u.message().chat().type() ) { //== fb::ChatRead::Type::privateChat 
    case fb::ChatRead::Type::group:
    case fb::ChatRead::Type::supergroup:
      if( u.message().text().decodeUnicode().equals( settings.getButtonName() )){
        // проверяем дату/время сообщения
        const time_t msgTimestamp = u.message().date();
        //const time_t _now = time(nullptr);
        auto timeNow = time(nullptr);
        debugPrintln( timeNow );
        debugPrintln( Time::toStr( timeNow ) );

        long delta = ( msgTimestamp > timeNow ) ? 
          msgTimestamp - timeNow : 
          timeNow - msgTimestamp;


        debugPrintf( "Msg timestamp=%lld, now=%lld, delta=%ld\n",
           msgTimestamp, timeNow, delta );
        
        // игнорируем сообщения старше 10 секунд
        if( delta > 10L ) return false;
        

        // открываем
        if ( relay.isAutocloseable() ){
          relay.open();    
        } else {
          debugPrintln( F("TODO: сделать сменную клаву"));
          // редактируем кнопку в группе
          if ( relay.isOpen() ){
            relay.close(); 
          } else {
            relay.open();
          }
        }                
        
        debugPrintln( F(" ставим лайк " ));
        setReaction( u.message(), "👌" );

        return true;
      }
  }  
  return false;
}

// bool checkBotCommand(fb::Update u){
//     bool hasName = flase;
//     size_t cmdHash;
//     uint16_t hasArgs;
//     Text arg;

//         Text text[2];
//         hasName = u.message().text().split( text, 2 , '@') == 2;
        
//         //auto cmdHash = hasBotName ? text[0].hash() : u.message().text().hash();

//         if ( hasName ){
//            cmdHash = text[0].hash();  
//           auto botRecepient = text[1].getSub(0, " ");
//            arg = text[1].getSub(1, " ");
//            hasArgs = text[1].count(" ");
          
//           debugPrintf("To bot '%s'\n", text[1].toString().c_str() );
//           debugPrintf("Cmd=%s %s\n", text[0].toString().c_str(), 
//               hasArgs > 1 ? arg.toString().c_str() : "");
//         } else {
//           Text cmd = text[0].getSub(0, " ");
//           cmdHash = cmd.hash();
//           arg = text[0].getSub(1, " ");
//           hasArgs = text[0].count(" ");
//           debugPrintf("Cmd=%s %s\n", cmd.toString().c_str(), 
//             hasArgs > 1 ? arg.toString().c_str() : "");
//         }

//               // игнорируем в групповом чате команды не нашему боту
//         if ( hasBotName && ! botName.equals( text[1].c_str() )) return;
//         if ( chat_id < 0 && !hasBotName ) return ;
//       }

void handleCommand(fb::Update& u){
  // игнорируем команды из группового чата
  if ( checkGroupChat( u ) ) return;

  auto msg = u.message();
  auto msgText = msg.text();
  
  // если не команда
  if( msgText.c_str()[0] != '/') return; 
  
  // готовим ответ
  fb::Message respond;
  respond.setModeMD();
// будем беседовать с отправителем
  respond.chatID = msg.from().id();

  

  debugPrintln(msg.from().username());
  debugPrintln(msgText);

        // Text arg;
  uint8_t parts = msgText.count(" ");
  bool hasArgs = parts > 1;
  Text cmd = msgText.getSub(0, " ");;
  Text arg;
  for ( uint8_t i=1; i< parts; i++ ) { 
    arg = msgText.getSub( i, " ");
    if ( arg.length() > 0 ) break;
  } 
      //auto cmdArgs = !hasBotName ? msgText.getSub(0, " ") : ;
  debugPrintf("Cmd=%s(%s)\n", cmd.toString().c_str(), 
            hasArgs ? arg.toString().c_str() : "null");

  size_t cmdHash = msgText.getSub(0, " ").hash();
    // команды для всех 

      switch( cmdHash ){
          case "/hi"_h:{
            respond.text = TelegramMD::asItallic( SAY_HI,  MARKDOWN_TG::escape );
          }
          break; 
        case "/start"_h:          
          handleStart(u, respond);  
          break;
        case "/version"_h:
          versionTo( respond );
          break;

        default: 
        
    // команды для только Админа    
        //bool freeOrAdmin = ! settings.hasAdmin() || settings.isAdmin( message.chatID );
          if ( !settings.hasAdmin() || settings.isAdmin( respond.chatID ) ){
      // if ( freeOrAdmin ){
            switch( cmdHash ){
              case "/time"_h:
                {
                  respond.text = MARKDOWN_TG::escape( Time::toStr() );
                  Time::_free_buf();
                }
                break;
              case "/help"_h: 
                sendHelp( respond );  
              
                break;

              case "/sysinfo"_h:
              {
                bot.setTyping(respond.chatID, false);
                String sysinfo; sysinfo.reserve(512);
                sysinfoTo(sysinfo);
                respond.text = sysinfo;
                respond.mode = fb::Message::Mode::Text;
              }
                break;

              case "/ls"_h:
                {
                  runStart;
                  bot.setTyping(respond.chatID, false);
                  String list; list.reserve(1024);
                  BotSettings::listDirTo(list, "/");
                  int startPtr = 0;
                  
                  while ( list.length() > 512 ){
                    int endPtr = startPtr + 512;  
                    respond.text = TelegramMD::asCode( list.substring(startPtr, endPtr ));
                    bot.sendMessage( respond, false );
                    startPtr = endPtr;
                    list = list.substring(endPtr );
                  }
                  respond.text += TelegramMD::asCode( BotSettings::listDirToString("/"));
                  // message.chatID = msg.from().id();
                  printRunTime;
                }
                break;
              case "/open_time"_h:
              {  
                if ( hasArgs ){
                  auto seconds = arg.toInt32();

                  if ( settings.set()->RelayPeriod( seconds ) ) {
                    settings.save();
                    relay.setOpenPeriod( seconds );
                    if ( relay.isAutocloseable() ){
                      myButton.txtChanger( nullptr);
                    } else {
                      myButton.txtChanger( isRelayOn );
                    }
                  }
                }
                
                //message.mode = fb::Message::Mode::Text;
                respond.text = F("Open period: ");
                respond.text += relay.getOpenPeriod();
                respond.text += F("sec");
                debugPrintln( respond.text );
              }
              break;
              
              case "/wifi_power"_h:
              { //.admin
                uint8_t percent = WiFiPower::wifiPower.getPower();
                bool willWrited = false;
                // if ( u.message().text().count(" ") >= 2 ) {
                //   auto arg = u.message().text().getSub(1, " ");
                if( hasArgs ){
                  //auto arg = u.message().text().getSub(1, " ");
                  if ( arg.valid() && is_digits( arg.toString().c_str() )) {
                      auto _arg = arg.toInt();
                      if ( _arg <= 100 ){
                        percent = WiFiPower::wifiPower.setPower( (uint8_t)_arg );
                        willWrited = !willWrited;
                      }
                  }
                }
                respond.text = F("Power: ");
                respond.text += percent;
                respond.text += "%\n";
                if ( willWrited )  respond.text += TelegramMD::asItallic( waitRecord,  MARKDOWN_TG::escape );
                else { 
                  auto dBm = WiFi.RSSI();
                  String rssi(dBm);
                    rssi += "dBm";
                    respond.text += TelegramMD::asBold( wm.getWiFiSSID().c_str() , MARKDOWN_TG::escape );
                    respond.text += PSTR("RSSI");
                    respond.text += TelegramMD::textIn_(  
                      ( IS_SIGNAL_GOOD(dBm) ) ? 
                        TelegramMD::asBold(rssi, MARKDOWN_TG::escape ) : 
                      ( IS_SIGNAL_POOR(dBm) ) ? 
                        TelegramMD::asItallic(rssi, MARKDOWN_TG::escape ) : 
                      MARKDOWN_TG::escape( rssi ),
                      '(',')', MARKDOWN_TG::escape );

                  }
                debugPrintln( respond.text );
              }
              break;

          //#define debug_print 
          //#ifdef debug_print
              case "/uptime"_h:
              {
                Time::uptimeTo(respond.text); 
                debugPrintln( respond.text );
              }
              break;

              case "/check_github"_h:
              { 
                    GitHubUpgrade::check( /*now=*/true);        
              }
              break; 
              case "/checked"_h:
                { 
                      auto arg = msgText.getSub(1, " ");
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
                      respond.mode = fb::Message::Mode::Text;
                      respond.text = F("Checked day=");
                      respond.text += GitHubUpgrade::at._checkedDay;
                      respond.text += '\n';
                      respond.text += GitHubUpgrade::at.toString();
                      debugPrintln(respond.text);
                  }
                  break;

                  case "/clear_settings"_h:
                    { //.admin ){
                      if ( settings.remove() ){
                        debugPrintln(F("Settings file deleted."));
                        settings.load();
                        respond.text += REBOOT_MD; //TelegramMD::asItallic( REBOOT,  MARKDOWN_TG::escape );
                        bot.sendMessage( respond );
                        respond.text = "";
                        //needStartPortal = true;
                        //needStart = NeedStart::Portal;
                        //delay(1000);
                        //bot.skipUpdates();
                        //bot.reboot();
                        needStart = NeedStartE::Reboot;  
                      } else {
                        debugPrintln(F("Error file deleted."));
                      }
                    }
                    break;
                  case "/clear_ignore"_h:
                    {
                      // if ( menuIds.has("ignore") ){
                      //   menuIds.remove("ignore");
                      // }
                      if ( menuIds.removeIgnoreVersion() ){
                        debugPrintln(F("Ignore version cleaned"));
                      }
                    }
                    break;
                  case "/clear_admin"_h:
                  if ( msg.from().id() == su::Admin ){ 
                      settings.set()->AdminId(0);
                      debugPrintln(F("Clear admin in RAM only"));
                      bot.deleteMyCommands(false);
                    }
                    break;
                  case "/reboot"_h:
                  { 
                    
                    //message.chatID = settings.getAdminId();
                    respond.text = REBOOT_MD; //TelegramMD::asItallic( REBOOT,  MARKDOWN_TG::escape ); //rebootMsg_MD;
                    //message.setModeMD;
                    bot.sendMessage(respond, true);
                    bot.setTyping( settings.getAdminId(), false);
                    respond.text = "";
                    //bot.reboot();
                    needStart = NeedStartE::Reboot; 
                  }
                    //ESP.restart();
                    break;
                  case "/clear_lastmsg"_h:
                    { 
                      // LastMsg l(msg.from().id());
                      // l.clean();
                      menuIds.removeMenuId(msg.from().id());
                    }
                    break;
                  case "/settings"_h:
                    { //.admin ){
                      
                      debugPrintln( settings ); //.toString());
                      debugPrintf("Last message for %lld is %lu\n", settings.getAdminId(), menuIds.getMenuId(settings.getAdminId()));  //LastMsg(settings.getAdminId()).get() );
                      if( settings.getChatId(true) != 0 )
                        debugPrintf("Last message for %lld is %lu\n", settings.getChatId(),  menuIds.getMenuId(settings.getChatId())); //LastMsg(settings.getChatId()).get() );
                      // LastMsg lm(msg.chat().id());
                      // debugPrint("LastMsg:"); debugPrintln(lm.get());
                    }
                    break;
                  case "/rm"_h:
                    if ( !hasArgs ) break;
                      else 
                    { 
                      //auto arg = msgText.getSub(1, " ");
                      respond.text += F("File ");
                      respond.text += TelegramMD::asCode( arg.c_str() ); //.c_str();
                      respond.text += ' ';
                      if( ! arg.valid() ||  ! LittleFS.exists(arg.c_str()) ){  
                        respond.text += F("not exist");  
                      } else {
                        auto res = LittleFS.remove( arg.c_str() );
                        if ( res ){
                          respond.text += F("deleted");
                        } else {
                          respond.text += F("error");
                        }
                      }
                    }
                    break;
                  case "/cat"_h:
                    { 
                      //if ( msgText.count(" ") < 2 ) break;
                      if ( !hasArgs ) break;
                      else {
                      //auto arg = msgText.getSub(1, " ");
                      bot.setTyping(settings.getAdminId(), false);
                        if( ! LittleFS.exists(arg.c_str()) ){
                          respond.text += F("File ");
                          respond.text += TelegramMD::asCode( arg ); //.c_str();
                          respond.text += F(" not exist");
                          break;
                        }
                        auto f = LittleFS.open(arg.c_str(), "r");
                        if ( ! f ) {
                          respond.text += F("Error open file "); //`");
                          respond.text += TelegramMD::asCode( arg ); //.c_str();
                          //message.text += '`';
                          break; 
                        } 
                        String s = f.readString();
                        f.close();

                        debugPrintln(s);
                        s.replace('\\','/');

                        respond.text += TelegramMD::asCode( s );
                      }
                    }
                    break;
          //#endif          
                  case "/starPortal"_h:
                  case "/start_portal"_h:
                    { //.admin ){

                      needStart = NeedStartE::Portal;
                      respond.text += TelegramMD::asItallic( portalStarted, MARKDOWN_TG::escape);

                    }
                    break;
                  case "/startWeb"_h:
                  case "/start_web"_h:
                    { //.admin ){
                      //if ( needStart == NeedStart::Web ){
                      
                      switch ( needStart) {
                        case NeedStartE::None:
                        needStart = NeedStartE::Web;

                        respond.text = TelegramMD::asItallic(
                          //String("Settings ") + 
                          TelegramMD::linkTo(
                            webPortal, //"web portal", 
                            WiFi.localIP().toString().c_str(),
                            /* mdnsStarted ? App::getHostname().c_str() : WiFi.localIP().toString().c_str(), */
                            MARKDOWN_TG::escape ) + 
                          MARKDOWN_TG::escape( _started ) //" started...")
                        );

                        debugPrintf( "web portal on: %s\n", 
                            WiFi.localIP().toString().c_str());

                        if ( bot.sendMessage(respond) ){
                          webPortalMsgId = bot.lastBotMessage();
                          //message.text = NULL_STR;
                        }
                        break;
                        
          //              case  NeedStart::Web:
                        default:
                          respond.text += webPortal;//F("Веб портал уже запущен");
                          respond.text += _started;
                          
                        // break;
                        // case  NeedStart::Portal:
                          
                        //   message.text += F("Captive портал уже запущен");
                        //   message.
                        // break;
                      }
                      if ( ! respond.text.isEmpty() ){
                        debugPrintln( respond.text);
                        respond.text = NULL_STR;
                      }
                    }
                    break;
                  case "/stopWeb"_h: 
                  case "/stop_web"_h:
                    { //.admin ){
                      if ( needStart == NeedStartE::WebRunning ){
                        needStart = NeedStartE::None;
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
                          respond.text += closed;
                        }
                      }
                    }
                    break;
                  case "/backup"_h:
                    {
                       //settingsBackup();
                        if ( ! Backup::settings() ){
                          respond.text ="TODO: release backup function";
                        }
                    } break;

                    // запускается по получению архива файла
                    // case "/restore"_h:
                  // {
                  //   if (settings.isAdmin( msg.from().id() ) ) //settingsBackup();
                  //     if ( ! Backup::settings() ){
                  //       message.text ="TODO: release backup function";
                  //     }
                  // } break;


                } //switch admin

          }
            
    }// switch all
      
    // если есть что ответить 
    if ( ! respond.text.isEmpty() ) {

      debugPrint("msg: ");
      debugPrintln( respond.text );
      debugPrint("To:");
      debugPrintln( respond.chatID );

      bot.sendMessage(respond, false);
    }
  } //startsWith '/'
//};


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
            TelegramMD::textIn_( newChatTitle, '\'' ),
          MARKDOWN_TG::escape );  
        message.chatID = settings.getAdminId();
        message.setModeMD();
        bot.sendMessage(message);
        debugPrintln( message.text );   
      } else {
        debugPrintln( u.message().text());
         
        if ( u.message().text().startsWith( "/open" )){
          handleCommand(u);
        }
      }
    }
    break;

  case fb::Update::Type::CallbackQuery: 
    {
    bool myAlert = false;
    
    bool needReport = false;
    auto sender = u.query().from();

    
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
          txt += TelegramMD::textIn_( menuIds.getChannelName( settings.getChatId(true) ), '\'' );
          myAlert = true;
        } else {
          // проверяем время на кнопке
          long buttonTime = resp.substring(strlen(ButtonInlineMenu::bCmds) );//constLength(QUERY_START_OPEN)).toInt32();
          if ( ! myButton.isExpired( buttonTime ) ){  
            if ( u.query().message().chat().type() != fb::ChatRead::Type::privateChat && 
                 u.query().from().username().length() == 0 ){
                  myAlert = true;
                  txt += F("Вы не можете пользоватся ботом со скрытым id!");
            } else {
              needReport = true;
              if ( relay.isAutocloseable() ){
                
                relay.open();    
                txt = settings.getButtonReport();
                getNameFromRead(txt, sender, (char *)F(", ") );
                //getNameFromRead(txt, u.message().from(), (char *)F(", ") ); 
              } else {
                // тут обновляем кнопку для режима on/off
                
                if ( relay.isOpen() ){
                  relay.close(); 
                } else {
                  relay.open();
                }
                myButton.updater(false );              
              }
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
    if ( needReport ) {
      sendReport( settings.getAdminId(), sender );
    }
  }
  break;
  default:
    Serial.println( String(F("Unknown fb::update type "))+(uint)u.type() );

  }
}

