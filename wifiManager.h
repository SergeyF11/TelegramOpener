#include "utils/string.h"
#pragma once

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <time.h>
#include <stdio.h>
#include <FastBot2.h>

#define USEOTA
// enable OTA
#ifdef USEOTA
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#endif
#include "String2int64.h"
#include "newFsSettings.h"
//#include "quotesReplace.h"
#include "simpleButton.h"

#define PortalWiFiPassword "12345678"
#define PORTAL_TIMEOUT 90

const char* modes[] = { "NULL", "STA", "AP", "STA+AP" };

unsigned long mtime = 0;
enum NeedStart {
  None = 0,
  Portal = 1,
  Web = 2,
  WebRunning = 3,
  WebStop = 4
};

NeedStart needStart=None;

WiFiManager wm;

class IntParameter : public WiFiManagerParameter {
public:
    IntParameter() : WiFiManagerParameter("") {};
    IntParameter(const char *id, const char *placeholder, long value, const uint8_t length = 10)
        : WiFiManagerParameter("") {
        init(id, placeholder, String(value).c_str(), length, "", WFM_LABEL_BEFORE);
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
        init(id, placeholder, int64ToString(value).c_str(), length, label, WFM_LABEL_BEFORE);
    };

    int64_t getValue() {
        return string2int64(WiFiManagerParameter::getValue());
    };
};


// TEST OPTION FLAGS
// bool TEST_CP         = false; //false; // always start the configportal, even if ap found
// int  TESP_CP_TIMEOUT = 90; // test cp timeout

// bool TEST_NET        = true; // do a network test after connect, (gets ntp time)
// bool ALLOWONDEMAND   = true; // enable on demand
// int  ONDDEMANDPIN    = 3; // RX


WiFiManagerParameter custom_html("<p>TG opener settings</p>"); // only custom html
WiFiManagerParameter custom_tgToken; //("token", "bot token", settings.token, 50,"placeholder=\"your BOT token\"");
Int64Parameter custom_botAdmin; //("adminId", "admin id", settings.admin, 21,"placeholder=\"bot administrator\"");
Int64Parameter custom_controlChatId; //("chatId", "control chat", settings.chatId, 21,"placeholder=\"token to access the HTTP API\"");
//WiFiManagerParameter custom_tgTokenb; //("invalid token", "invalid token", "", 0); // id is invalid, cannot contain spaces

WiFiManagerParameter custom_timeZone;
WiFiManagerParameter button_header;
WiFiManagerParameter button_name;
WiFiManagerParameter button_report;


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

String getNameByChipId(const char* baseName="TelegramOpener_")  {
  String name(baseName);
  char buf[10];
  sprintf(buf, "%05x", ESP.getChipId());
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
//   Serial.println("[CALLBACK] saveParamCallback fired");
//   // wm.stopConfigPortal();
// }

extern BotSettings::Settings settingsNew;
extern SimpleButton myButton;
extern FastBot2 bot;

void saveParamCallback() {
  debugPretty;
  settingsNew.set()->AdminId(custom_botAdmin.getValue());
  settingsNew.set()->ChatId(custom_controlChatId.getValue());

  if( settingsNew.set()->Token(custom_tgToken.getValue()) ) {
    debugPretty;
    bot.setToken(settingsNew.getToken());
  }
  if( settingsNew.set()->Tz(custom_timeZone.getValue()) ) { 
    debugPretty;
    settingsNew.configTz(); 
  }

  // settingsNew.set()->ButtonHeader(button_header.getValue());
  // settingsNew.set()->ButtonName(button_name.getValue());
  // settingsNew.set()->ButtonReport(button_report.getValue());
  if ( settingsNew.set()->Button(
      button_header.getValue(), 
      button_name.getValue(), 
      button_report.getValue()) 
      ) {
    debugPretty;    
    myButton.needUpdate(SimpleButton::NeedUpdate::setTrue);
  }

  debugPrintln( settingsNew );
  if ( ! settingsNew.save() ){
    debugPrintln("Error write settings file");
  }

};

void bindServerCallback(){
  wm.server->on("/custom",handleRoute);
  // wm.server->on("/info",handleRoute); // you can override wm!
}

void handleRoute(){
  debugPrintln("[HTTP] handle route");
  wm.server->send(200, "text/plain", "hello from user code");
}

void wifiInfo(){
  WiFi.printDiag(Serial);
  debugPrintln("SAVED: " + (String)wm.getWiFiIsSaved() ? "YES" : "NO");
  debugPrintln("SSID: " + (String)wm.getWiFiSSID());
  debugPrintln("PASS: " + (String)wm.getWiFiPass());
}
//==================================================================

