#include "utils/string.h"
#pragma once
#define WM_STRINGS_FILE "wm_strings_ru.h"
//#define WIFI_MANAGER_OVERRIDE_STRINGS

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <time.h>
#include <stdio.h>
#include "myFastBotClient.h"

#define USEOTA
// enable OTA
#ifdef USEOTA
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#endif
//#include "String2int64.h"
#include "newFsSettings.h"
//#include "quotesReplace.h"
#include "simpleButton.h"
#include "relay.h"

extern Relay relay;

static const char PortalWiFiPassword[] PROGMEM = "12345678";
#define PORTAL_TIMEOUT 90

//const char* modes[] PROGMEM = { "NULL", "STA", "AP", "STA+AP" };

//unsigned long mtime = 0;
enum NeedStartE {
  Set = -1,
  None = 0,
  Portal = 1,
  Web = 2,
  WebRunning = 3,
  WebStop = 4,
  Reboot,
  CertsDownloading,
  GetRSSI,
};

NeedStartE needStart=None;

// namespace ns{
//   int needStart=None;
// }
// NeedStartE needStartF(NeedStartE set = NeedStartE::Set ){
//   if( set == NeedStartE::Set )  { 
//     ns::needStart = ns::needStart | (1<<set);
//     return None;
//   }
//   if ( set & ns::needStart ) return set;
//   else return None;
// } 

WiFiManager wm; //(Serial); 

class IntParameter : public WiFiManagerParameter {
public:
    IntParameter() : WiFiManagerParameter("") {};
    IntParameter(const char *id, const char *placeholder, long value, const uint8_t length = 10, const char * label = "")
        : WiFiManagerParameter("") {
        init(id, placeholder, String(value).c_str(), length, label, WFM_LABEL_BEFORE);
    }

    long getValue() {
        return String(WiFiManagerParameter::getValue()).toInt();
    }
};

class Int64Parameter : public WiFiManagerParameter {
public:
    Int64Parameter() : WiFiManagerParameter("") {};
    Int64Parameter(const char *id, const char *placeholder, int64_t value, const uint8_t length = 21, const char *label= "")
        : WiFiManagerParameter("") {
        init(id, placeholder, su::Value(value).c_str(), length, label, WFM_LABEL_BEFORE);
    };

    int64_t getValue() {
        //return string2int64(WiFiManagerParameter::getValue());
        return su::Text( WiFiManagerParameter::getValue()).toInt64();
    };
};


// TEST OPTION FLAGS
// bool TEST_CP         = false; //false; // always start the configportal, even if ap found
// int  TESP_CP_TIMEOUT = 90; // test cp timeout

// bool TEST_NET        = true; // do a network test after connect, (gets ntp time)
// bool ALLOWONDEMAND   = true; // enable on demand
// int  ONDDEMANDPIN    = 3; // RX


WiFiManagerParameter custom_html( "<h1>TelegramOpener</h1>" ); // only custom html
WiFiManagerParameter custom_tgToken; //("token", "bot token", settings.token, 50,"placeholder=\"your BOT token\"");
Int64Parameter custom_botAdmin; //("adminId", "admin id", settings.admin, 21,"placeholder=\"bot administrator\"");
Int64Parameter custom_controlChatId; //("chatId", "control chat", settings.chatId, 21,"placeholder=\"token to access the HTTP API\"");
//WiFiManagerParameter custom_tgTokenb; //("invalid token", "invalid token", "", 0); // id is invalid, cannot contain spaces

WiFiManagerParameter custom_timeZone;
WiFiManagerParameter button_header;
WiFiManagerParameter button_name;
WiFiManagerParameter button_report;
IntParameter relay_period;


void saveWifiCallback();
void handleRoute();
void configModeCallback (WiFiManager *myWiFiManager);
void saveParamCallback();
void bindServerCallback();
void wifiInfo();

void printParam( const WiFiManagerParameter& param)  {
  debugPrintln("Get Params:");
  debugPrint(param.getID());
  debugPrint(" : ");
  debugPrintln(param.getValue());
}

String getNameByChipId(const char* baseName=nullptr)  {
  String name(baseName);
  char buf[10];
  sprintf(buf, "_%05x", ESP.getChipId());
  name += buf;
  return name;
}
void saveWifiCallback(){
  debugPrintln("[CALLBACK] saveCallback fired");
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  //debugPrintln("[CALLBACK] configModeCallback fired");
  debugPretty;
}

// void saveParamCallback(){
//   Serial.println("[CALLBACK] saveParamCallback fired");period=3000ms
//   // wm.stopConfigPortal();
// }

extern BotSettings::Settings settings;
extern SimpleButton myButton;
extern FastBot2Client bot;

void saveParamCallback() {
  debugPretty;
  settings.set()->AdminId(custom_botAdmin.getValue());
  settings.set()->ChatId(custom_controlChatId.getValue());

  if( settings.set()->Token(custom_tgToken.getValue()) ) {
    debugPretty;
    bot.setToken(settings.getToken());
  }
  if( settings.set()->Tz(custom_timeZone.getValue()) ) { 
    debugPretty;
    settings.configTz(); 
  }
  if ( settings.set()->RelayPeriod(relay_period.getValue())){
    debugPretty;
    relay.setOpenPeriod( settings.getRelayPeriod());
  }
  // settings.set()->ButtonHeader(button_header.getValue());
  // settings.set()->ButtonName(button_name.getValue());
  // settings.set()->ButtonReport(button_report.getValue());
  if ( settings.set()->Button(
      button_header.getValue(), 
      button_name.getValue(), 
      button_report.getValue()) 
      ) {
    debugPretty;    
    myButton.needUpdate(true);
  }

  debugPrintln( settings );
  if ( ! settings.save() ){
    debugPrintln(F("Error write settings file"));
  }

};

void bindServerCallback(){
  wm.server->on("/custom",handleRoute);
  // wm.server->on("/info",handleRoute); // you can override wm!
}

void handleRoute(){
  debugPrintln(F("[HTTP] handle route"));
  wm.server->send(200, "text/plain", "hello from user code");
}

void wifiInfo(){
  WiFi.printDiag(Serial);
  debugPrintln("SAVED: " + (String)wm.getWiFiIsSaved() ? "YES" : "NO");
  debugPrintln("SSID: " + (String)wm.getWiFiSSID());
  debugPrintln("PASS: " + (String)wm.getWiFiPass());
}
//==================================================================

