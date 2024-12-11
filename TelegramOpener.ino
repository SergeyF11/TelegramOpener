//#define debug_print 1
//#define SYNC_TIME

#define HW_622
//#define TEST_WEMOS

#define SEC *1000
#define POLLING_TIME 10 SEC
//#define BUTTON_ENABLE_SEC 41
//#define POLLING_TIME (BUTTON_ENABLE_SEC-1)*500
#define RX_PIN 3


#include "env.h"
#include <Arduino.h>
#include "my_credential.h"
#include "debug.h"

#include <time.h>
#include "relay.h"
#include <FastBot2.h>
#include "channelName.h"
#include "newFsSettings.h" 
FastBot2 bot;
Relay relay(RELAY_PORT, RELAY_INIT_STATUS, 3);


#include "updateh.h"

// #include "old/expireButton.h"
// ExpireButton myButton(BUTTON_ENABLE_SEC);

#include "simpleButton.h"
SimpleButton myButton(bot, POLLING_TIME );

#include "wifiManager.h"

//const char *TZstr = DEFAULT_TZ_STR;

//String jsonSettings;
//fb::Result res;
//uint wrongCount =0;
//SETTINGS::SettingsT settings;
const char fileName[] PROGMEM = "/bot_opener.json";
BotSettings::Settings settingsNew(fileName);

LastMsg lastMsg;
//void * buttonUpdater;
void rawResponse(su::Text resp){
  debugPretty;   debugPrintln(resp.c_str());
  if (resp.valid()) wrongCount.reset(); 
  else wrongCount++;
};

void setup(){
    pinMode(RX_PIN,INPUT);
    WiFi.mode(WIFI_STA);
    if (WiFi.getPersistent() == true) WiFi.persistent(false); 
    WiFi.setOutputPower( 0 );

    Serial.begin(115200);
    while ( ! Serial ){
      delay(10);
    }
    delay(1000);
    Serial.println();
    debugBegin(115200);
//=========================================
    // SETTINGS::fsInit();

    // String settingsJson = SETTINGS::getJson();
    // if ( settingsJson.isEmpty() ) {
    //   settingsJson = SETTINGS::createDefault();
    // }
    // SETTINGS::parse(&settings, settingsJson);


// setup some parameters
  //new (&custom_html ) WiFiManagerParameter ("<p>Bot settings</p>"); // only custom html
  new (&custom_tgToken ) WiFiManagerParameter ("token", "bot token", settingsNew.getToken(), 50,"placeholder=\"your BOT token from @BotFather\"");
  //new (&custom_tgTokenb ) WiFiManagerParameter ("invalid token", "invalid token", "", 0); // id is invalid, cannot contain spaces
  
  new (&custom_botAdmin ) Int64Parameter ("adminId", "admin id", settingsNew.getAdminId(), 21,"placeholder=\"bot administrator id\"");
  new (&custom_controlChatId ) Int64Parameter ("chatId", "control chat id", settingsNew.getChatId(true), 21,"placeholder=\"control chat id\"");
  new (&custom_timeZone) WiFiManagerParameter ("tz", "time zone", settingsNew.getTz(), 10, "placeholder=\"time zone (example:'MSK-3') or shift in hours (-3)\"");

  new (&button_header) WiFiManagerParameter ("button_header","Button header", settingsNew.getButtonHeader(), 150, "placeholder=\"Button message header\"");
  new (&button_name) WiFiManagerParameter ("button_name","Button name", settingsNew.getButtonName(), 50, "placeholder=\"Button name\"");
  new (&button_report) WiFiManagerParameter ("button_report","Open report", settingsNew.getButtonReport(), 100, "placeholder=\"Open report message\"");

  // WiFiManagerParameter button_header;
  // WiFiManagerParameter button_name;
  // WiFiManagerParameter button_report;

  //WiFiManagerParameter custom_timeZone("tz", "time zone", "MSK-3", 10);
  //WiFiManagerParameter custom_ipaddress("input_ip", "input IP", "", 15,"pattern='\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}'"); // custom input attrs (ip mask)

  // const char _customHtml_checkbox[] = "type=\"checkbox\""; 
  // WiFiManagerParameter custom_checkbox("checkbox", "my checkbox", "T", 2, _customHtml_checkbox, WFM_LABEL_AFTER);

  // add all your parameters here
  wm.addParameter(&custom_html);
  wm.addParameter(&custom_tgToken);
  //wm.addParameter(&custom_tgTokenb);
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
  //std::vector<const char *> menu = {"wifi","wifinoscan","info","param","close","sep","erase","update","restart","exit"};
  std::vector<const char *> menu = {"wifi","info","sep","param","sep","update","restart","exit"};
  wm.setMenu(menu); // custom menu, pass vector
  //wm.setCountry("US"); 

  // set Hostname
  wm.setHostname(F("TelegramOpener"));
  //useful to make it all retry or go to sleep in seconds
  //wm.setConfigPortalTimeout(120);

  wm.setConfigPortalTimeout(PORTAL_TIMEOUT);
  wm.setSaveConnect(true); // false = ( do not connect, only save )
  wm.setBreakAfterConfig(true); // needed to use saveWifiCallback
   wifiInfo();


  pinMode(RX_PIN, INPUT_PULLUP);
  builtInLed.on();
  if(!wm.autoConnect(getNameByChipId().c_str(), PortalWiFiPassword )) {
    Serial.println(F("\nfailed to connect and hit timeout"));
  }
  else if( digitalRead(RX_PIN) == 0 ) {
  //   // start configportal always
     
     wm.startConfigPortal(getNameByChipId().c_str(), PortalWiFiPassword );
   }
  else {
    //if you get here you have connected to the WiFi
     Serial.println("connected...yeey :)");
  }

  bot.attachUpdate(updateh);   // подключить обработчик обновлений
  bot.setToken( settingsNew.getToken() );   // установить токен
  bot.skipUpdates();
  bot.attachRaw(rawResponse);

  //check token
  while( ! bot.tickManual() ) {
    debugPrintln("Wrong token");
    
    wm.startConfigPortal(getNameByChipId().c_str(), PortalWiFiPassword );
    //bot.setToken( settingsNew.getToken() ); 
  }
  // else {
  debugPrintln("Updates received. Token ok.");
  //}

  bool goToLoop = false;

  // если есть админ, поприветствуем его и обновим клавиатуру или создадим новую
  if ( settingsNew.getAdminId() != 0 ){
    fb::Message message;
    message.chatID = settingsNew.getAdminId();
    message.setModeMD();

    message.text = F(SAY_HI_MD);

    channelName::load(settingsNew.getChatId(true));
    if ( settingsNew.getChatId(true) ) {
      //message.text += F("\nМой канал управления *");
      message.text += '\n';
      message.text += CHANNEL_FOR_CONTROL;
      message.text += F("*");
      if ( channelName::isEmpty() ){
        message.text += "\\#\\"; 
//                                                    1001715239030ll
        message.text += 1000000000000ll + settingsNew.getChatId(true);
      } else { 
        message.text += F("\\'");
        message.text += channelName::get(); 
        message.text += F("\\'");
      }
      message.text += F("*");
    }

    debugPrint("Say hi: ");
    debugPrintln(message.text);
    auto res = bot.sendMessage(message);
    debugPrintln( res.getRaw() );
    channelName::freeMemory();

    while( ! goToLoop ){
  // check errors noChat, noMesgId, wrongResponse
      switch(myButton.updater( settingsNew.getChatId(), settingsNew.getButton(), true ) ){
        // сюда мы не должны попасть никогда
        case SimpleButton::ReturnCode::noChat: 
          debugPrintln("No update needed without admin and chat id");
          goToLoop = true;
          break;
        // у чата нет последнего сохраненного сообщения
        case SimpleButton::ReturnCode::noMesgId:
          debugPrintf("No last message saved for chat '%lld\nTry to create new one\n", settingsNew.getChatId());
          // пробуем создать новою клавиатуру
          if ( myButton.creater( settingsNew.getChatId(), settingsNew.getButton() ) ) 
            goToLoop = true;
          else {
            debugPrint("Wrong chat id ");
          }
          break;

        // не получили корректного ответа
        case SimpleButton::ReturnCode::wrongResponse:
          debugPrintf("Wrong response for chat '%lld' msg=%ld\n", 
            settingsNew.getChatId(), 
            LastMsg(settingsNew.getChatId() ).get());
          break;
        
        // в остальных случаях погнали дальше
        default:
          goToLoop = true;
      }

      // при наличии проблемы вызываем портал для настройки
      if( ! goToLoop ) {          
        wm.startConfigPortal(getNameByChipId().c_str(), PortalWiFiPassword );
        
      } 
    }
  }

    //digitalWrite(BUILTIN_LED, HIGH);
  builtInLed.off();
  wifiInfo();
  Serial.println(settingsNew);

    // ============
 
    //bot.tickManual();

  bot.setPollMode(fb::Poll::Long, POLLING_TIME);
    
// Sync time 
#ifdef SYNC_TIME 
    Serial.print(F("Sync time "));
    while( time(nullptr)< 3600 ){        
      //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        builtInLed.flash(200,1);
        delay(10);
        Serial.print("+");
    }
    Serial.println(F("Done"));
#endif

//  если есть админ, то нужно обновлять кнопку у админа или в чате
  if( settingsNew.getChatId() != 0ll )
    myButton.needUpdate( SimpleButton::NeedUpdate::setTrue );

  debugPrintMemory;
}

//bool needStartPortal = false;

//bool needPrintMemory = false;

void loop() {
  _loop();
}



void _loop(){

  printMemory.tick(Serial);
  wrongCount.tick();

  relay.tick();
  bot.tick();

  if ( bot.canReboot() ) {
    ESP.restart(); 
  }


  if ( ! bot.isPolling() ) {
    
    // обновляем клаву без ожидания ответа
    //myButton.needUpdate(true);
    myButton.tick( settingsNew );
    // myButton.updater(bot, 
    //   settingsNew.getChatId(),
    //   settingsNew.getButton() /*, false */);
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
        wm.startConfigPortal(getNameByChipId().c_str(), PortalWiFiPassword );

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
    }
    // if ( needStartPortal ){
    //   bot.skipNextMessage(); 
    //   if( bot.tickManual() ) debugPrintln(F("Manual update done"));
    //   else debugPrintln(F("Error manual update"));
      
    //   Serial.println("\nPortal ENABLED");
    //   //needStartPortal = ! needStartPortal;
    //   builtInLed.on();       
    //   wm.setConfigPortalTimeout(PORTAL_TIMEOUT);
    //   wm.startConfigPortal(getNameByChipId().c_str(), PortalWiFiPassword );

    //   // check telegram answer
    //   if( ! bot.tickManual() ) {
    //     debugPrintln("Wrong answer == wrong token. Reboot...");
    //     ESP.restart();
    //   }
    //   //if( settings.chat.id == 0LL ) settings.chat.id = settings.admin;
    //   builtInLed.toggle();
    //   myButton.needUpdate( SimpleButton::NeedUpdate::setTrue );

    //   fb::Message message;
    //   message.chatID = settingsNew.getAdminId();
    //   message.text = F("_portal closed_");
    //   message.setModeMD();
    //   bot.sendMessage(message, false);
    // } 

  // #if defined debug_print
  //  else if ( needStartPortal ) { debugPrintln(F("Waiting polling release...")); }
  // #endif
  //}
}
