//#define debug_print 1
//#define memory_print
//#define CHECK_MAXBLOCK_SIZE
//#define certStoreUpdateTest

#define VERSION 0,1,23

#ifdef CHECK_MAXBLOCK_SIZE
  #define maxblock_size_checker { static uint32_t __pre_free_block=0; \
  if ( __pre_free_block - ESP.getMaxFreeBlockSize() > 1000 ){ \
  Serial.printf( "%lu in %d of %s: Pre free block size=%lu now %lu\n", millis()/1000, __LINE__, __PRETTY_FUNCTION__, __pre_free_block, ESP.getMaxFreeBlockSize()); } \
  __pre_free_block=ESP.getMaxFreeBlockSize(); } 
#else
  #define maxblock_size_checker
#endif
 //#define GitHubUpgrade_ANY_TIME



#define MFLN_SIZE 1024
#define SYNC_TIME

#ifdef debug_print 
#define TEST_WEMOS
#define WIFI_POWER 8.0
#else 
#define HW_622
#define WIFI_POWER 18.0
#endif

#define SEC *1000
#define POLLING_TIME 20 SEC
//#define BUTTON_ENABLE_SEC 41
//#define POLLING_TIME (BUTTON_ENABLE_SEC-1)*500
#define RX_PIN 3


#include "env.h"
#include "debug.h"

#if defined debug_print
  PrintMemory memory;
  static App::Version version{VERSION,"dbg"};
#else
  static App::Version version{VERSION};
 #ifdef memory_print 
  PrintMemory memory;
  #endif  
#endif

#include <Arduino.h>
#include "my_credential.h"

#include <time.h>
#include "relay.h"
#include "myFastBotClient.h"
//#include <FastBot2Client.h>

//#include "channelName.h"
#include "newFsSettings.h" 
#include "myPairs.h"
//#include "myFileDb.h"
#include "github_upgrade.h"
//FastBot2 bot;
Relay relay(RELAY_PORT, RELAY_INIT_STATUS, 3);
static bool isRelayOn(){ return relay.isOpen(); };

#include "updateh.h"

// #include "old/expireButton.h"
// ExpireButton myButton(BUTTON_ENABLE_SEC);

#include "simpleButton.h"
#include "wifiManager.h"
#include "rssi.h"

static const char fileName[] PROGMEM = "/bot_opener.json";
BotSettings::Settings settings(fileName);
SimpleButton myButton(bot, settings, POLLING_TIME );

/// @brief все настройки скетча
void setup(){
  
  Serial.begin(115200);
    while ( ! Serial ){
      delay(1);
    }
  menuIds.begin();
  myButton.setExtraTime(6000);
  relay.setOpenPeriod( settings.getRelayPeriod() );
  if ( !relay.isAutocloseable() )
    myButton.txtChanger( isRelayOn );
  
    pinMode(RX_PIN, INPUT_PULLUP);
    WiFi.mode(WIFI_STA);
    if (WiFi.getPersistent() == true) WiFi.persistent(false); 
#if defined WIFI_POWER
    //
    if ( ! WiFiPower::wifiPower.read() ) {
      debugPrintln("Saved wifi power date not found");
    }
    {
    auto power = WiFiPower::wifiPower.setPower();
    debugPrintf("\nPower set to %u%\n", power );
    }
    // else 
    //   WiFi.setOutputPower( WIFI_POWER );
#endif


    //delay(1000);
    Serial.println();
    Serial.println(App::appVersion(version));
    
    
    //debugBegin(115200);
//=========================================
    // SETTINGS::fsInit();

    // String settingsJson = SETTINGS::getJson();
    // if ( settingsJson.isEmpty() ) {
    //   settingsJson = SETTINGS::createDefault();
    // }
    // SETTINGS::parse(&settings, settingsJson);


// setup some parameters

  new (&custom_tgToken ) WiFiManagerParameter ("token", "Bot token", settings.getToken(), 50,"placeholder=\"your BOT token from @BotFather\"");
  new (&custom_botAdmin ) Int64Parameter ("adminId", "Admin ID", settings.getAdminId(), 21,"placeholder=\"bot administrator id\"");
  new (&custom_controlChatId ) Int64Parameter ("chatId", "Control chat ID", settings.getChatId(true), 21,"placeholder=\"control chat id\"");
  new (&custom_timeZone) WiFiManagerParameter ("tz", "Time zone", settings.getTz(), 10, "placeholder=\"time zone (example:'MSK-3') or shift in hours (-3)\"");

  new (&button_header) WiFiManagerParameter ("button_header","Button header", settings.getButtonHeader(), 150, "placeholder=\"Button message header\"");
  new (&button_name) WiFiManagerParameter ("button_name","Button name", settings.getButtonName(), 50, "placeholder=\"Button name\"");
  new (&button_report) WiFiManagerParameter ("button_report","Open report or name", settings.getButtonReport(), 100, "placeholder=\"Open report message or button name\"");
  new (&relay_period) IntParameter ( "relay_period", "Relay open seconds", settings.getRelayPeriod() , 10 ,  "placeholder=\"relay on (seconds), 0 for manual mode\""); 
 
  // add all your parameters here
  wm.addParameter(&custom_html);
  wm.addParameter(&custom_tgToken);
  wm.addParameter(&custom_botAdmin);
  wm.addParameter(&custom_controlChatId);
  wm.addParameter(&custom_timeZone);

wm.addParameter(&button_header);
wm.addParameter(&button_name);
wm.addParameter(&button_report);
wm.addParameter(&relay_period);


  // callbacks
  wm.setAPCallback(configModeCallback);
  wm.setWebServerCallback(bindServerCallback);
  wm.setSaveConfigCallback(saveWifiCallback);
  wm.setSaveParamsCallback(saveParamCallback);
 
  wm.setDebugOutput( false); //true, WM_DEBUG_DEV );
  // invert theme, dark
  wm.setDarkMode(true);
   std::vector<const char *> menu = {"wifi","info","sep","param","sep","update","restart","exit"};
  wm.setMenu(menu); // custom menu, pass vector
 
  // set Hostname
  wm.setHostname( App::name );

  // useful to make it all retry or go to sleep in seconds
  wm.setConfigPortalTimeout( settings.hasToken() ? PORTAL_TIMEOUT : (PORTAL_TIMEOUT<<8) );
  wm.setSaveConnect(false); // false = ( do not connect, only save )
  wm.setBreakAfterConfig(true); // needed to use saveWifiCallback
   wifiInfo();

  //wm.setDebugOutput(true, WM_DEBUG_DEV);

  
  builtInLed.on();
  if( digitalRead(RX_PIN) == 0 ) {
  //   // start configportal always  
     wm.startConfigPortal(getNameByChipId(App::name).c_str(), PortalWiFiPassword );
  }
  while( !wm.autoConnect(getNameByChipId(App::name).c_str(), PortalWiFiPassword )) {
    Serial.println(F("\nfailed to connect and hit timeout"));
  } 
    
    //if you get here you have connected to the WiFi
     Serial.println("connected...yeey :)");


  //static esp8266::polledTimeout::periodicMs [](){ };
 // Sync time 
  #ifdef SYNC_TIME 
  // sntp_set_sync_interval(  60UL * 1000 );
  // sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
  
      Serial.print(F("Wait sync time "));

      while( ! Time::isSynced() ){        
          builtInLed.flash(200,1);
          delay(10);
          Serial.print("+");
      }
//      sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);

      Serial.println( Time::toStr() ); //Time::printTo(Serial);
      //Time::_free_buf();
      debugPrintln(F(" Done" ));

  #endif
  // certStore = botCertsStore(client, LittleFS);
  // if ( certStore != nullptr) {
  if ( botCertsStore( certStore, client, LittleFS) ){
    debugPrintf("Use certs store [%llu]\n", certStore );
    #ifdef certStoreUpdateTest
      FileTime::setModificated( LittleFS,  CertStoreFiles::fileData, 1739548570 );
      #pragma message("=======================================================")
      #pragma message("======= for debug only!! Change CertStore data ========")
      #pragma message("=======================================================")

    #endif
  } else {
    Serial.println("No certificate store loaded!! Reset esp.");
    Serial.flush();
    ESP.reset();
    //client.setInsecure();
    //client.setFingerprint(Telegram::fingerprint );
    //debugPrintln("Use Telegram fingerprint\n");
  }
  // } else {
  //   client.setInsecure();
  //   debugPrintln("Use insecure Telegram commection\n");
  // }

  bot.attachUpdate(updateh);   // подключить обработчик обновлений
  bot.setToken( settings.getToken() );   // установить токен
  //bot.skipNextMessage();
  bot.skipUpdates();
  bot.attachRaw(rawResponse);

  //check token
  // void wrongToken(){
  //     debugPrintf("Wrong token %s\n", settings.getToken());    
  //     wm.startConfigPortal(getNameByChipId(App::name).c_str(), PortalWiFiPassword );
  // };
  // wrongCount.accidentFunc( wrongToken );
  
  wrongCount.accidentFunc = [](void){
    debugPrintf("Wrong token %s\n", settings.getToken());   
    builtInLed.on();
    wm.startConfigPortal(getNameByChipId(App::name).c_str(), PortalWiFiPassword );
    wrongCount.reset();
  };
  
  while( ! bot.tickManual() ) {  
    delay(100);
    wrongCount.tick();
    builtInLed.flash(300, 50);
    //wm.startConfigPortal(getNameByChipId(App::name).c_str(), PortalWiFiPassword );
  }
  // else {
  wrongCount.setWrongPeriod(POLLING_TIME*15);
  debugPrintln("Updates received. Token ok.");
  wrongCount.accidentFunc = ESP.restart;
  //}

  bool goToLoop = false;
  

  // если есть админ, поприветствуем его и обновим клавиатуру или создадим новую
  if ( settings.hasAdmin() ){
    myButton.needUpdate( true );
    fb::Message message;
    message.setModeMD();
    message.text = TelegramMD::asItallic( SAY_HI, MARKDOWN_TG::escape);
    
    if ( settings.getChatId(true) != 0 ) {
      message.text += TelegramMD::newLine(); //'\n';
      message.text +=  CHANNEL_FOR_CONTROL;

      if( menuIds.hasChannelName(settings.getChatId(true)) /*myChnlName.isEmpty()*/ ) {
        message.text += TelegramMD::asBold( 
          TelegramMD::textIn( 
            (String)menuIds.getChannelName(settings.getChatId(true)), '\'' ),
          MARKDOWN_TG::escape);  
      } else {
        message.text += TelegramMD::asBold( 
            String('#') + (1000000000000ll + settings.getChatId(true)), 
          MARKDOWN_TG::escape);
      }
    }

    message.chatID = settings.getAdminId();
    //message.text += myChannel;

    debugPrint( F("Say hi: "));
    debugPrintln( message.text );
    debugPrint(F(" to ")); debugPrintln( settings.getAdminId());
    debugPrintln( message.chatID );

    bot.sendMessage(message, false);
    
    //String chat= message.chatID;
    debugPrintf("+++++++++\nchat=\'%s\' message.text = \"%s\"\n+++++++++\n", 
      message.chatID.toString().c_str(), 
      message.text.c_str());
    debugPrintln(message.chatID);

    //channelName::freeMemory();
  }
  while( ! goToLoop ){
  // check errors noChat, noMesgId, wrongResponse

      switch( myButton.updater( true )){
        // сюда мы не должны попасть никогда
        case SimpleButton::ReturnCode::noChat: 
          debugPrintln("No update needed without admin and chat id");
          goToLoop = true;
          break;
        // или у чата нет последнего сохраненного сообщения
        case SimpleButton::ReturnCode::noMesgId:
          debugPrintf("No last message saved for chat '%lld\n", settings.getChatId());
        // нет корректного ответа 
        case SimpleButton::ReturnCode::wrongResponse:
          debugPrintln("Try to create new keyboard");
            // пробуем создать новою клавиатуру
            {
              int trys=3;
              while( trys-- ){  
                delay(1500);
                auto res = myButton.creater(); //settings.getChatId(), settings.getButton() );
                if ( res != SimpleButton::ReturnCode::wrongResponse ) {
                  goToLoop = true;
                  break;
                }  
              }
              // не получили корректного ответа
              if( ! goToLoop ) {
                debugPrintf("Wrong response 3 times for create button in chat '%lld'\n", 
                  settings.getChatId());
              }
            }
          //goToLoop = true;  
          break;

        // в остальных случаях погнали дальше
        default:
          goToLoop = true;
      }

      // при наличии проблемы вызываем портал для настройки
      if( ! goToLoop ) {          
        wm.startConfigPortal(getNameByChipId(App::name).c_str(), PortalWiFiPassword );
        
      } 
    }
  
  relay.setOpenPeriod( settings.getRelayPeriod() );
  builtInLed.off();
  wifiInfo();
  Serial.println(settings);

    // ============
 
    //bot.tickManual();

  bot.setPollMode(fb::Poll::Long, POLLING_TIME);

 
//GitHubUpgrade::initOtaUpgrade(LittleFS);

#if defined debug_print or defined GitHubUpgrade_ANY_TIME
  //GitHubUpgrade::checkAt( GitHubUpgrade::At::Any, GitHubUpgrade::At::Any, GitHubUpgrade::At::Any );
  GitHubUpgrade::at.set(  GitHubUpgrade::At::Any, GitHubUpgrade::At::Any, GitHubUpgrade::At::Any );
  //debugPrintln("Check upgrade done");
  // delay(1000);
  // CertStoreFiles::updatedMsg(bot, settings.getAdminId(), true );
  //FileTime::setModificated( LittleFS, CertStoreFiles::fileData, 5555555 );
   

#else
  //GitHubUpgrade::checkAt( GitHubUpgrade::At::Random(7) );
  GitHubUpgrade::at.set( GitHubUpgrade::At::Random(7) );
#endif


} // end setup()


void loop(){
#ifdef memory_print
  if( memory.needPrint() ) { 
    //memory.printTo(Serial); 
    Serial.println( memory );
    //memory.needPrint(false); 
  }
#endif
  {
    wrongCount.tick();
    relay.tick();
  }
    bot.tick() ;
  {
    menuIds.tick();
  }
  {
    if ( WiFiPower::wifiPower.isWrited() ){
      fb::Message message;
      //message.text = myChannel;
      message.text = TelegramMD::asBold( wifiPowerWrited, MARKDOWN_TG::escape );  
      message.chatID = settings.getAdminId();
      message.setModeMD();
      bot.sendMessage(message);
    }
  }

  maxblock_size_checker;
 

  if ( ! bot.isPolling() ) {
  //  myButton.tick();
  //} else {
    GitHubUpgrade::tick();
    myButton.tick();

    if( CertStoreFiles::hasNewestCertsStore() ){
      CertStoreFiles::downloadMsg(bot, settings.getAdminId(), true );
      debugPrintf("Github certStore date is %s\n", Time::toStr( GitHubUpgrade::release._newCertStoreDate ));
      debugPrintf("Current certStore date is %s\n", Time::toStr( CertStoreFiles::fileDate(LittleFS ) ));
    
      //bot.tickManual();
      if ( CertificateStore::update( GitHubUpgrade::release ) ){
        debugPrintf("Certs store update to date %s\n", Time::toStr( GitHubUpgrade::release._newCertStoreDate )); 
        auto res = CertStoreFiles::updatedMsg(bot, settings.getAdminId(), true );
        //bot.tickManual();
        debugPrintln( res ? F("success") : F(" error send msg"));

        if ( CertificateStore::upgrade() ){
          //bot.reboot();
          debugPrintln("Reboot...");
          Serial.flush();
          delay(1);
          ESP.restart();
        }
      } 
    }
  }

  if ( bot.canReboot() ) {
    debugPrintln("Reboot esp...");
    Serial.flush();
    delay(1);
    ESP.restart(); 
  }

  switch ( needStart )
    {
    case NeedStartE::Portal:
      if( ! bot.isPolling() ) {

        if( bot.tickManual() ) debugPrintln(F("Manual update done"));
        else debugPrintln(F("Error manual update"));
        
        Serial.println("\nPortal ENABLED");
        //needStartPortal = ! needStartPortal;
        builtInLed.on();       
        wm.setConfigPortalTimeout(PORTAL_TIMEOUT);
        wm.startConfigPortal(getNameByChipId(App::name).c_str(), PortalWiFiPassword );

        // check telegram answer
        if( ! bot.tickManual() ) {
          debugPrintln("Wrong answer == wrong token. Reboot...");
          ESP.restart();
        }
        //if( settings.chat.id == 0LL ) settings.chat.id = settings.admin;
        
        // это не обязательно
        //builtInLed.off();
        myButton.needUpdate( true );

        fb::Message message;
        message.chatID = settings.getAdminId();
        message.text = F("_portal closed_");
        message.setModeMD();
        bot.sendMessage(message, false);
        needStart = NeedStartE::None; 
      }
      break;
    case NeedStartE::Web:
      //webPortalMsgId = bot.lastBotMessage();
      wm.startWebPortal();
      needStart = NeedStartE::WebRunning;
    
      break;
    case NeedStartE::WebStop:
      wm.stopWebPortal();
      needStart = NeedStartE::None;
      break;
    case NeedStartE::WebRunning:
      builtInLed.flash(400, 200);
      wm.process();
      if ( ! wm.getWebPortalActive() ) {
        if ( webPortalMsgId ){
          //bot.deleteMessage ( settings.getAdminId(), webPortalMsgId, false );
          fb::TextEdit editMsg;
          editMsg.chatID = settings.getAdminId();
          editMsg.text = F("_portal closed_");
          editMsg.mode = fb::Message::Mode::MarkdownV2;
          editMsg.messageID = webPortalMsgId;
          bot.editText(editMsg, false );
          webPortalMsgId = 0;
        }
        needStart = NeedStartE::None; 
      }  
      break;
    case NeedStartE::Reboot:
      if( bot.isPolling() ) {
        bot.skipNextMessage();
        bot.skipUpdates();
        bot.tickManual();
        debugPrintln("Reboot...");
        Serial.flush();
        delay(1);
        ESP.restart();
      }
      break;
    // case NeedStart::CertsDownloading:
    //   CertificateStore::Updater certsUpdater();
    // case NeedStartE::GetRSSI:
    //   if( bot.isPolling() ){
    //       if ( RSSI::sendReport() )
    //         needStart = NeedStartE::None;
    //   }
    //   break;
    }
   
}
