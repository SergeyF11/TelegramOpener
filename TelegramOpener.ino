 #define debug_print 1
 #define GitHubUpgrade_ANY_TIME

#define WIFI_POWER 5.0

#define MFLN_SIZE 1024
#define SYNC_TIME

#define HW_622
//#define TEST_WEMOS

#define SEC *1000
#define POLLING_TIME 20 SEC
//#define BUTTON_ENABLE_SEC 41
//#define POLLING_TIME (BUTTON_ENABLE_SEC-1)*500
#define RX_PIN 3

#define VERSION 0,1,0
#include "env.h"
#if defined debug_print
  static App::Version version{VERSION,"dbg"};
#else
  static App::Version version{VERSION};
#endif

#include <Arduino.h>
#include "my_credential.h"
#include "debug.h"
#include "github_upgrade.h"
#include <time.h>
#include "relay.h"
#include "myFastBotClient.h"
//#include <FastBot2Client.h>

//#include "channelName.h"
#include "newFsSettings.h" 
#include "myPairs.h"
//#include "myFileDb.h"

//FastBot2 bot;
Relay relay(RELAY_PORT, RELAY_INIT_STATUS, 3);

#include "updateh.h"

// #include "old/expireButton.h"
// ExpireButton myButton(BUTTON_ENABLE_SEC);

#include "simpleButton.h"
SimpleButton myButton(bot, POLLING_TIME );

#include "wifiManager.h"

const char fileName[] PROGMEM = "/bot_opener.json";
BotSettings::Settings settingsNew(fileName);

//LastMsg lastMsg;



//void * buttonUpdater;
void rawResponse(su::Text resp){
  //debugPrint(__TIME__ ); debugPrint(' ');
  debugPretty;   // debugPrintln(resp.c_str());
  if (resp.valid()) wrongCount.reset(); 
  else 
    wrongCount++;
};

void setup(){
  Serial.begin(115200);
    while ( ! Serial ){
      delay(1);
    }
  menuIds.begin();
  myButton.setExpiredDelta(5000);

    pinMode(RX_PIN,INPUT);
    WiFi.mode(WIFI_STA);
    if (WiFi.getPersistent() == true) WiFi.persistent(false); 
#if defined WIFI_POWER
    WiFi.setOutputPower( WIFI_POWER );
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
  new (&custom_tgToken ) WiFiManagerParameter ("token", "bot token", settingsNew.getToken(), 50,"placeholder=\"your BOT token from @BotFather\"");
   new (&custom_botAdmin ) Int64Parameter ("adminId", "admin id", settingsNew.getAdminId(), 21,"placeholder=\"bot administrator id\"");
  new (&custom_controlChatId ) Int64Parameter ("chatId", "control chat id", settingsNew.getChatId(true), 21,"placeholder=\"control chat id\"");
  new (&custom_timeZone) WiFiManagerParameter ("tz", "time zone", settingsNew.getTz(), 10, "placeholder=\"time zone (example:'MSK-3') or shift in hours (-3)\"");

  new (&button_header) WiFiManagerParameter ("button_header","Button header", settingsNew.getButtonHeader(), 150, "placeholder=\"Button message header\"");
  new (&button_name) WiFiManagerParameter ("button_name","Button name", settingsNew.getButtonName(), 50, "placeholder=\"Button name\"");
  new (&button_report) WiFiManagerParameter ("button_report","Open report", settingsNew.getButtonReport(), 100, "placeholder=\"Open report message\"");

 
  // add all your parameters here
  wm.addParameter(&custom_html);
  wm.addParameter(&custom_tgToken);
  wm.addParameter(&custom_botAdmin);
  wm.addParameter(&custom_controlChatId);
  wm.addParameter(&custom_timeZone);

wm.addParameter(&button_header);
wm.addParameter(&button_name);
wm.addParameter(&button_report);


  // callbacks
  wm.setAPCallback(configModeCallback);
  wm.setWebServerCallback(bindServerCallback);
  wm.setSaveConfigCallback(saveWifiCallback);
  wm.setSaveParamsCallback(saveParamCallback);
 
  
  // invert theme, dark
  wm.setDarkMode(true);
   std::vector<const char *> menu = {"wifi","info","sep","param","sep","update","restart","exit"};
  wm.setMenu(menu); // custom menu, pass vector
 
  // set Hostname
  wm.setHostname(App::name);
  //useful to make it all retry or go to sleep in seconds
  //wm.setConfigPortalTimeout(120);

  wm.setConfigPortalTimeout(PORTAL_TIMEOUT);
  wm.setSaveConnect(true); // false = ( do not connect, only save )
  wm.setBreakAfterConfig(true); // needed to use saveWifiCallback
   wifiInfo();


  pinMode(RX_PIN, INPUT_PULLUP);
  builtInLed.on();
  if(!wm.autoConnect(getNameByChipId(App::name).c_str(), PortalWiFiPassword )) {
    Serial.println(F("\nfailed to connect and hit timeout"));
  }
  else if( digitalRead(RX_PIN) == 0 ) {
  //   // start configportal always
     
     wm.startConfigPortal(getNameByChipId(App::name).c_str(), PortalWiFiPassword );
   }
  else {
    //if you get here you have connected to the WiFi
     Serial.println("connected...yeey :)");
  }
 // Sync time 
  #ifdef SYNC_TIME 
      Serial.print(F("Sync time "));
      while( ! Time::isSynced() ){        
          builtInLed.flash(200,1);
          delay(10);
          Serial.print("+");
      }
      
      debugPrintln( Time::toStr() ); //Time::printTo(Serial);
      debugPrintln(F(" Done"));
      //debugPrintln( Time );
      //Serial.print()
  #endif
  certStore = botCertsStore(client, LittleFS);
  if ( certStore != nullptr) {
    debugPrintf("Use certs store [%llu]\n", certStore );
  } else {
    debugPrintln("Use Telegram fingerprint\n");
  }
  // } else {
  //   client.setInsecure();
  //   debugPrintln("Use insecure Telegram commection\n");
  // }

  bot.attachUpdate(updateh);   // подключить обработчик обновлений
  bot.setToken( settingsNew.getToken() );   // установить токен
  bot.skipUpdates();
  bot.attachRaw(rawResponse);

  //check token
  // void wrongToken(){
  //     debugPrintf("Wrong token %s\n", settingsNew.getToken());    
  //     wm.startConfigPortal(getNameByChipId(App::name).c_str(), PortalWiFiPassword );
  // };
  // wrongCount.accidentFunc( wrongToken );
  
  wrongCount.accidentFunc = [](void){
    debugPrintf("Wrong token %s\n", settingsNew.getToken());    
    wm.startConfigPortal(getNameByChipId(App::name).c_str(), PortalWiFiPassword );
  };

  while( ! bot.tickManual() ) {  
    delay(100);
    wrongCount.tick();
    //wm.startConfigPortal(getNameByChipId(App::name).c_str(), PortalWiFiPassword );
  }
  // else {
  debugPrintln("Updates received. Token ok.");
  wrongCount.accidentFunc = ESP.restart;
  //}

  bool goToLoop = false;
  

  // если есть админ, поприветствуем его и обновим клавиатуру или создадим новую
  if ( settingsNew.hasAdmin() ){
    myButton.needUpdate( true );

    // channelName::load(settingsNew.getChatId(true));
    //String myChnlName = channelName::addChannelName( settingsNew.getChatId(true), '\n' );
    String myChannel;

    if ( settingsNew.getChatId(true) != 0 ) {
      myChannel += '\n';
      myChannel +=  CHANNEL_FOR_CONTROL;

      //String myChnlName; // = menuIds.getChannelName(settingsNew.getChatId(true)); //menuIds.get( 'n', settingsNew.getChatId(true));
      if( menuIds.hasChannelName(settingsNew.getChatId(true)) /*myChnlName.isEmpty()*/ ) {
        myChannel += TelegramMD::asBold( 
          TelegramMD::textIn( 
            (String)menuIds.getChannelName(settingsNew.getChatId(true)), '\'' ),
          MARKDOWN_TG::escape);  
      } else {
        myChannel += TelegramMD::asBold( String('#') + (1000000000000ll + settingsNew.getChatId(true)),   MARKDOWN_TG::escape);
      }
    }
    String hi = TelegramMD::asItallic( SAY_HI, MARKDOWN_TG::escape);
    
    fb::Message message;
    message.setModeMD();

    message.chatID = settingsNew.getAdminId();
    message.text = hi;
    //message.text += TelegramMD::asItallic( SAY_HI ); //*/ SAY_HI_MD;
    message.text += myChannel;

    debugPrint("Say hi: ");
    debugPrintln( message.text );
    
    bot.sendMessage(message, false);

    debugPrintf("message.text = \"%s\"\n", message.text.c_str());

    //channelName::freeMemory();
 }
    while( ! goToLoop ){
  // check errors noChat, noMesgId, wrongResponse

      switch( myButton.updater( settingsNew.getChatId(), settingsNew.getButton(), true ) ){
        // сюда мы не должны попасть никогда
        case SimpleButton::ReturnCode::noChat: 
          debugPrintln("No update needed without admin and chat id");
          goToLoop = true;
          break;
        // или у чата нет последнего сохраненного сообщения
        case SimpleButton::ReturnCode::noMesgId:
          debugPrintf("No last message saved for chat '%lld\n", settingsNew.getChatId());
        // нет корректного ответа 
        case SimpleButton::ReturnCode::wrongResponse:
          debugPrintln("Try to create new keyboard");
            // пробуем создать новою клавиатуру
            {
            auto res = myButton.creater( settingsNew.getChatId(), settingsNew.getButton() );
            debugPrintf("Result: %s\n", myButton.codeToString(res).c_str());
            if ( res != SimpleButton::ReturnCode::wrongResponse ) goToLoop = true;
            }
          //goToLoop = true;  
          break;

        // не получили корректного ответа
        // case SimpleButton::ReturnCode::wrongResponse:
        //   debugPrintf("Wrong response for chat '%lld' msg=%ld\n", 
        //     settingsNew.getChatId(), 
        //     LastMsg(settingsNew.getChatId() ).get());
        //   break;
        
        // в остальных случаях погнали дальше
        default:
          goToLoop = true;
      }

      // при наличии проблемы вызываем портал для настройки
      if( ! goToLoop ) {          
        wm.startConfigPortal(getNameByChipId(App::name).c_str(), PortalWiFiPassword );
        
      } 
    }
  

    //digitalWrite(BUILTIN_LED, HIGH);
  builtInLed.off();
  wifiInfo();
  Serial.println(settingsNew);

    // ============
 
    //bot.tickManual();

  bot.setPollMode(fb::Poll::Long, POLLING_TIME);

 
//GitHubUpgrade::initOtaUpgrade(LittleFS);

#if defined debug_print or defined GitHubUpgrade_ANY_TIME
  GitHubUpgrade::checkAt( GitHubUpgrade::At::Any, GitHubUpgrade::At::Any, GitHubUpgrade::At::Any );
  //debugPrintln("Check upgrade done");
#else
  GitHubUpgrade::checkAt( );
#endif

//bool needStartPortal = false
//bool needPrintMemory = false;
} // end setup()


void loop(){

  printMemory.tick(Serial);
  wrongCount.tick();

  relay.tick();
  bot.tick();
  menuIds.tick();

  myButton.tick( settingsNew );

  
  if ( bot.canReboot() ) {
    debugPrintln("Reboot esp...");
    Serial.flush();
    delay(1);
    ESP.restart(); 
  }


  if ( ! bot.isPolling() ) {
    GitHubUpgrade::tick( );
    //GitHubUpgrade::OtaClean(true);
  }  

 
  switch ( needStart )
    {
    case NeedStart::Portal:
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
        builtInLed.toggle();
        myButton.needUpdate( SimpleButton::NeedUpdate::setTrue );

        fb::Message message;
        message.chatID = settingsNew.getAdminId();
        message.text = F("_portal closed_");
        message.setModeMD();
        bot.sendMessage(message, false);
        needStart = NeedStart::None; 
      }
      break;
    case NeedStart::Web:
      //webPortalMsgId = bot.lastBotMessage();
      wm.startWebPortal();
      needStart = NeedStart::WebRunning;
    
      break;
    case NeedStart::WebStop:
      wm.stopWebPortal();
      break;
    case NeedStart::WebRunning:
      builtInLed.flash(50);
      wm.process();
      if ( ! wm.getWebPortalActive() ) {
        if ( webPortalMsgId ){
          //bot.deleteMessage ( settingsNew.getAdminId(), webPortalMsgId, false );
          fb::TextEdit editMsg;
          editMsg.chatID = settingsNew.getAdminId();
          editMsg.text = F("_portal closed_");
          editMsg.mode = fb::Message::Mode::MarkdownV2;
          editMsg.messageID = webPortalMsgId;
          bot.editText(editMsg, false );
          webPortalMsgId = 0;
        }
        needStart = NeedStart::None; 
      }
      break;
    case NeedStart::Reboot:
      if( bot.isPolling() ) {
        bot.skipUpdates();
        //bot.tickManual();
        debugPrintln("Reboot...");
        Serial.flush();
        delay(1);
        ESP.reset();
      }
      break;
    }
   
}
