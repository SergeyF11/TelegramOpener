#include <cstddef>
//#include "core_esp8266_features.h"
#pragma once
#include <Arduino.h>

class DebugInit {
  public:
  DebugInit(HardwareSerial& port=Serial, long baud=115200){
    port.begin(baud);
    while ( ! port ){
      delay(10);
    }
    delay(1000);
  }
};

class PrintMemory : public Printable {
    private:
    bool _needPrint=false;
    
    public:
    PrintMemory() {};
    void needPrint(bool need=true){
      this->_needPrint=need;
    };
    bool isNeedPrint() const {
      return this->_needPrint;
    };
    void tick(Print& p) {
      if ( this->_needPrint) {
        p.println(*this);
        needPrint(false);
      }
    };
    String toString() const {
      String s(F("Free heap="));
      s += ESP.getFreeHeap();
      s += F("\nMax free block=");
      s += ESP.getMaxFreeBlockSize();
      return s;
    };
    size_t printTo(Print& p) const {
      return p.print(this->toString());
    };
  } printMemory;

#if defined debug_print
   DebugInit debugInit(Serial, 115200);
   #define debugBegin(x)        if(! Serial) Serial.begin(x)
   #define debugPrint(x)              Serial.print(x)
   #define debugPrintln(x)            Serial.println(x)
   #define debugPrintf(s,...)          { Serial.printf((s), __VA_ARGS__); }
   #define debugPretty              Serial.print(__LINE__); Serial.print(" "); Serial.println(__PRETTY_FUNCTION__)
   #define runStart                 Serial.println(F("Start loop ms"));RunTimeMs _finish; 
   #define printRunTime                  { auto stop=_finish.time(); Serial.print(F("Loop timeMs=")); Serial.println(stop); }
  #define debugPrintMemory Serial.println(printMemory)
  #define debugBotResult(res,msg) { if( res.valid()) { debugPrint( msg); debugPrintln(" done"); } else { debugPrint("ERROR: "); debugPrintln( msg );} }
  
//   #define debugPrintMemory
#else
   #define debugBegin(x)
   #define debugPrint(x)
   #define debugPrintln(x)
   #define debugPrintf(s,...)
   #define debugPretty
   #define runStart
   #define printRunTime
   #define debugPrintMemory
   #define debugBotResult(res,msg)
#endif


class RunTimeMs : public Printable {
  public:
  RunTimeMs(){
    this->start = millis();  
    debugPretty;  
  };
  unsigned long time() const {
    return millis()-this->start;
  };
  size_t printTo(Print& p) const {
    return p.println(this->time());
  };
  private:
  unsigned long start;
};