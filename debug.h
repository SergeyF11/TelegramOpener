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

/// @brief класс для печати свободной памяти ESP и максимального блока
class PrintMemory : public Printable {
    private:
    bool _needPrint=false;
    
    public:
    PrintMemory() {};
    /// @brief  invert _needPrint if it was true
    /// @return return previusly value _needPrint
    inline bool needPrint(){
      bool ret = _needPrint;
      if ( _needPrint ){
        _needPrint = ! _needPrint;
      }
      return ret;
    }
    /// @brief setter for _needPrint
    /// @param need true or false
    /// @return _needPrint
    inline bool needPrint(bool need){
      return this->_needPrint=need;
      //return _needPrint;
    };
    // inline bool isNeedPrint() const {
    //   return this->_needPrint;
    // };

    // оператор приведение типа
    // приводим к bool
    operator bool() const { return _needPrint; };

    // bool tick() {
    //   if ( this->_needPrint) {
    //     needPrint(false);
    //     return true;
    //   }
    //   return false;
    // };

    String toString() const {
      String s(F("Free heap="));
      s += ESP.getFreeHeap();
      s += F("\nMax free block=");
      s += ESP.getMaxFreeBlockSize();
      return s;
    };
// объявляем дружественный оператор для класса String
    friend String& operator +=(String& s, const PrintMemory& m );

    /// @brief print memory to Print
    /// @param p PrintMemory
    /// @return printed chars
    size_t printTo(Print& p) const {
      return p.print(this->toString());
    };
  };

// определяем оператор +=  для класса String с аргументом PrintMemory 

  /// @brief concat String and PrintMemory
  /// @param s String&
  /// @param m PrintMemory&
  /// @return concated string
  String& operator+=(String& s, const PrintMemory& m ){
    s += m.toString();
    return s;
  };

#if defined debug_print
   DebugInit debugInit(Serial, 115200);
   #define debugBegin(x)        if(! Serial) Serial.begin(x)
   #define debugPrint(x)         Serial.print(x)
   #define debugPrintln(x)       Serial.println(x)
   //#define debugPrintf(s,...)   { Serial.printf((s), __VA_ARGS__); }
   #define debugPrintf(fmt, ...) {  Serial.printf("%d %s # ", __LINE__, __PRETTY_FUNCTION__); Serial.printf_P( (PGM_P)PSTR(fmt), ## __VA_ARGS__ ); }
   #define debugPretty          Serial.print(__LINE__); Serial.print(" "); Serial.println(__PRETTY_FUNCTION__)
   #define runStart             Serial.print(__PRETTY_FUNCTION__); Serial.println(F(": Start loop ms"));RunTimeMs _finish
   #define printRunTime         auto stop=_finish.time();Serial.print(F("Loop timeMs="));Serial.println(stop)
  #define debugPrintMemory      { Serial.println(memory); }
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



#ifdef CHECK_MEMORY_LEAK
  #define MEMORY_CHECKER \
    do { \
      static uint32_t prev_free = 0; \
      static uint32_t prev_max = 0; \
      uint32_t curr_free = ESP.getFreeHeap(); \
      uint32_t curr_max = ESP.getMaxFreeBlockSize(); \
      uint32_t now = millis() / 1000; \
      \
      /* Проверка на явное повреждение кучи */ \
      if (curr_max > curr_free || curr_free > 50000) { \
        Serial.printf("[%lu] CRITICAL: Heap corrupted! free=%lu max=%lu at %s:%d\n", \
                      now, curr_free, curr_max, __FILE__, __LINE__); \
      } \
      \
      /* Утечка: свободная память уменьшилась более чем на 2 КБ */ \
      if (prev_free != 0 && prev_free - curr_free > 2048) { \
        Serial.printf("[%lu] LEAK: Free heap dropped by %ld bytes (was %lu, now %lu) at %s:%d\n", \
                      now, prev_free - curr_free, prev_free, curr_free, __FILE__, __LINE__); \
      } \
      \
      /* Фрагментация: максимальный блок уменьшился более чем на 1 КБ */ \
      if (prev_max != 0 && prev_max - curr_max > 1024) { \
        Serial.printf("[%lu] FRAG: Max block dropped by %ld bytes (was %lu, now %lu) at %s:%d\n", \
                      now, prev_max - curr_max, prev_max, curr_max, __FILE__, __LINE__); \
      } \
      \
      /* Освобождение: память увеличилась более чем на 2 КБ */ \
      if (prev_free != 0 && curr_free - prev_free > 2048) { \
        Serial.printf("[%lu] FREE: Heap increased by %ld bytes (was %lu, now %lu) at %s:%d\n", \
                      now, curr_free - prev_free, prev_free, curr_free, __FILE__, __LINE__); \
      } \
      \
      prev_free = curr_free; \
      prev_max = curr_max; \
    } while(0)
#else
  #define MEMORY_CHECKER
#endif


#ifdef CHECK_MAXBLOCK_SIZE
  #define maxblock_size_checker { static uint32_t __pre_free_block=0; \
  if ( __pre_free_block - ESP.getMaxFreeBlockSize() > 1000 ){ \
  Serial.printf( "%lu in %d of %s: Pre free block size=%lu now %lu\n", millis()/1000, __LINE__, __PRETTY_FUNCTION__, __pre_free_block, ESP.getMaxFreeBlockSize()); } \
  __pre_free_block=ESP.getMaxFreeBlockSize(); } 
#else
  #define maxblock_size_checker
#endif