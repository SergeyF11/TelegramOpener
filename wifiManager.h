#include "utils/string.h"
#pragma once
//#define WM_STRINGS_FILE "wm_strings_ru.h"
//#define WIFI_MANAGER_OVERRIDE_STRINGS

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <time.h>
#include <stdio.h>
#include "myFastBotClient.h"
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include "Updater.h"

//#define USEOTA
// enable OTA
#ifdef USEOTA

#include <ArduinoOTA.h>
#endif
//#include "String2int64.h"
#include "newFsSettings.h"
//#include "quotesReplace.h"
#include "simpleButton.h"
#include "relay.h"

extern Relay relay;

static const char PortalWiFiPassword[] PROGMEM = "12345678";
#define PORTAL_TIMEOUT ( 3*60UL )


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

const char MY_JSCRIPT_CODE[] PROGMEM = "<script>if(window.location.href.includes('save') || " \
    "window.location.href.includes('exit'))" \
    "{setTimeout(function(){window.location.href='/';}, 2000);}" \
    "</script>";

// const char PROGRESS_JSCRIPT[] PROGMEM = R"=====(
// <script>
// // Получаем текущий URL
// var currentUrl = window.location.pathname;
// console.log("Current URL:", currentUrl);

// // Если мы на странице обновления прошивки
// if(currentUrl.indexOf('/update') !== -1) {
//   console.log("На странице обновления, добавляем индикацию...");
  
//   // Стили для прогресс-индикации
//   var style = document.createElement('style');
//   style.textContent = `
//     .ota-progress {
//       margin: 30px 0;
//       text-align: center;
//     }
//     .ota-progress-bar {
//       width: 0%;
//       height: 20px;
//       background: #2196F3;
//       border-radius: 10px;
//       transition: width 0.3s;
//       margin: 10px auto;
//       max-width: 500px;
//       position: relative;
//       overflow: hidden;
//     }
//     .ota-progress-bar:after {
//       content: '';
//       position: absolute;
//       top: 0;
//       left: -100%;
//       width: 100%;
//       height: 100%;
//       background: linear-gradient(90deg, transparent, rgba(255,255,255,0.4), transparent);
//       animation: ota-shimmer 2s infinite;
//     }
//     @keyframes ota-shimmer {
//       100% { left: 100%; }
//     }
//     .ota-status {
//       color: #666;
//       font-size: 14px;
//       margin: 10px 0;
//     }
//     .ota-flashing {
//       animation: ota-flash 1s infinite alternate;
//       color: #ff9800;
//       font-weight: bold;
//     }
//     @keyframes ota-flash {
//       from { opacity: 1; }
//       to { opacity: 0.5; }
//     }
//     .ota-warning {
//       background: #fff3cd;
//       border: 1px solid #ffeaa7;
//       border-radius: 5px;
//       padding: 10px;
//       margin: 20px 0;
//       color: #856404;
//     }
//   `;
//   document.head.appendChild(style);
  
//   // Создаем контейнер для прогресса
//   var progressHTML = `
//   <div class="ota-progress">
//     <div class="ota-warning ota-flashing">
//       ⚡ ВНИМАНИЕ: Во время прошивки не отключайте питание!
//     </div>
//     <div class="ota-progress-bar" id="otaProgressBar"></div>
//     <div class="ota-status" id="otaStatus">
//       Готов к прошивке. Выберите файл и нажмите "Update"
//     </div>
//     <div id="otaLog" style="text-align:left; font-family:monospace; font-size:12px; max-height:200px; overflow-y:auto;"></div>
//   </div>`;
  
//   // Находим форму и вставляем прогресс перед ней
//   var form = document.querySelector('form[action*="update"]');
//   if(form) {
//     form.insertAdjacentHTML('beforebegin', progressHTML);
    
//     // Перехватываем отправку формы
//     form.addEventListener('submit', function(e) {
//       console.log("Форма отправляется...");
      
//       // Показываем прогресс
//       document.getElementById('otaStatus').textContent = "Начало прошивки...";
//       document.getElementById('otaStatus').classList.add('ota-flashing');
      
//       // Блокируем кнопку
//       var submitBtn = form.querySelector('input[type="submit"]');
//       if(submitBtn) {
//         submitBtn.disabled = true;
//         submitBtn.value = "Прошивка...";
//       }
      
//       // Начинаем имитацию прогресса
//       simulateOtaProgress();
      
//       // ДОПОЛНИТЕЛЬНО: Можно добавить XMLHttpRequest для отслеживания реального прогресса
//       // Но для простоты оставим имитацию
      
//       // Форма продолжит отправку стандартным способом
//       // Имитация прогресса будет работать параллельно
//     });
//   }
  
//   // Функция имитации прогресса прошивки
//   function simulateOtaProgress() {
//     var progress = 0;
//     var stages = [
//       {p: 5, msg: "Получение файла прошивки..."},
//       {p: 15, msg: "Проверка файла..."},
//       {p: 25, msg: "Валидация прошивки..."},
//       {p: 35, msg: "Подготовка памяти..."},
//       {p: 45, msg: "Стирание старой прошивки..."},
//       {p: 60, msg: "Запись новой прошивки..."},
//       {p: 75, msg: "Проверка записи..."},
//       {p: 85, msg: "Верификация данных..."},
//       {p: 95, msg: "Финальные настройки..."},
//       {p: 100, msg: "Прошивка завершена! Перезагрузка..."}
//     ];
    
//     var currentStage = 0;
//     var logElement = document.getElementById('otaLog');
    
//     function addLog(message) {
//       if(logElement) {
//         var now = new Date();
//         var time = ('0' + now.getHours()).slice(-2) + ':' + 
//                    ('0' + now.getMinutes()).slice(-2) + ':' + 
//                    ('0' + now.getSeconds()).slice(-2);
//         var logEntry = document.createElement('div');
//         logEntry.textContent = '[' + time + '] ' + message;
//         logElement.appendChild(logEntry);
//         logElement.scrollTop = logElement.scrollHeight;
//       }
//     }
    
//     function updateProgress() {
//       if(currentStage < stages.length) {
//         var stage = stages[currentStage];
//         progress = stage.p;
        
//         // Обновляем прогресс-бар
//         var progressBar = document.getElementById('otaProgressBar');
//         if(progressBar) {
//           progressBar.style.width = progress + '%';
//         }
        
//         // Обновляем статус
//         var status = document.getElementById('otaStatus');
//         if(status) {
//           status.textContent = stage.msg;
//         }
        
//         addLog(stage.msg);
        
//         currentStage++;
        
//         // Задержка между этапами
//         var delay = 300 + Math.random() * 700;
//         setTimeout(updateProgress, delay);
//       } else {
//         // Прошивка "завершена"
//         addLog("✅ Прошивка успешно установлена!");
//         addLog("🔄 Устройство перезагружается...");
        
//         // Меняем цвет прогресс-бара на зеленый
//         var progressBar = document.getElementById('otaProgressBar');
//         if(progressBar) {
//           progressBar.style.background = '#4CAF50';
//         }
        
//         // Через 3 секунды перенаправляем на главную
//         setTimeout(function() {
//           window.location.href = '/';
//         }, 3000);
//       }
//     }
    
//     addLog("🚀 Начало процесса прошивки...");
//     updateProgress();
//   }
// }
// </script>
// )=====";


//extern Led builtInLed; 

// ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
//   builtInLed.toggle();
//   // Инвертируем состояние встроенного LED (обычно пин 2)
//   //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
// });

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

void handlePreOtaUpdateCallback(){
  Update.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("CUSTOM Progress: %u%%\r", (progress / (total / 100)));
      digitalWrite(LED_BUILTIN, (progress & 1)  );
  });
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
  debugPrintf( "Hostname: %s\n", WiFi.getHostname());
}
//==================================================================

