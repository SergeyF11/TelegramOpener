// #include "core_esp8266_features.h"
// #include "common.h"
#pragma once
#include <time.h>
#ifdef TEST_WEMOS
#define RELAY_PORT LED_BUILTIN
#define RELAY_INIT_STATUS HIGH
#endif
#ifdef HW_622
#define RELAY_PORT 4
#define RELAY_INIT_STATUS LOW
#define INPUT_PORT 5
#endif

#ifndef RELAY_PORT
#warning Must be defined TEST_WEMOS or HW_622 for define RELAY_PORT
#endif

#define DEFAULT_CLOSED_SEC 1
//#define LOW 0
//#define HIGH 1
#define DEBUG_RELAY 0

#define BULITIN_LED_MACRO(period, flash, init) { \
  static int initStatus, status = init;     \
  static unsigned long t = 0UL;     \
  unsigned long now; now=millis();       \
  uint p = (status!=initStatus) ? (uint)(period-flash) : (uint)(flash); \
  if( now - t >= p ) { /*status==initStatus ? (unsigned long)(period) : (unsigned long)(flash) ) {   */     \
    if( Serial ) Serial.printf("Change status %d for %ld\n",status,now); \
    status = !status; \
    digitalWrite(BUILTIN_LED, status);                     \
    t = now;  }                \
}

class BuildInLed {
  public:
  // create led for pin
  BuildInLed(int pin){
    this->pin =pin;
    this->initStatus=digitalRead(this->pin);
    pinMode(this->pin,OUTPUT);
  };
  //get current led status
  inline int status(){ return digitalRead(this->pin); };
  //switch led on
  inline void on(){ digitalWrite(this->pin, !this->initStatus);};
  //switch led off
  inline void off() { digitalWrite(this->pin, this->initStatus);};
  void flash(uint periodMs=1000, uint flashMs=1) {
    unsigned long currentMs = millis(); 
    if( this->status() == this->initStatus ){
      if( currentMs - this->changeStatusMs < (periodMs-flashMs) ) return;
    } else {
      if( currentMs - this->changeStatusMs < flashMs ) return;
    }
    this->changeStatusMs = currentMs;
    digitalWrite(this->pin, ! this->status());
  };
  void flashOff(){
    if ( this->status() != this->initStatus ){
      this->off();
      this->changeStatusMs = millis();
    }
  };
  // switch led status
  void toggle(int period=0) {
    unsigned long currentMs = millis(); 
    if ( period ){
      if ( currentMs - this->changeStatusMs < period ) return;
    }
    this->changeStatusMs = currentMs;
    digitalWrite(this->pin, ! this->status());
    };

  private:
  int pin;
  int initStatus;
  unsigned long changeStatusMs=0;
} builtInLed(LED_BUILTIN);

//flasher for led
inline void ledFlash(uint period){ builtInLed.flash(period); };
//inline void flash200() { builtInLed.flash(200);}


struct WrongCount {
//  uint periodMs=1000;
//  long unsigned nextMs;
  uint count=0;
  unsigned long changeMs =0; 
  uint accident;
  void (*func)(uint);
  void (*accidentFunc)(void);

  //WrongCount(){ };
  WrongCount(void (*f)(uint), void (*af)(), const uint accident=3) { //}, const uint periodMs=10000 ) {
    this->accident=accident;
    this->func = f;
    this->accidentFunc = af;
  //  this->periodMs=periodMs;
  //  this->nextMs=millis()+periodMs;
  };

  uint get(){return this->count; };
  bool isWrong(){ return this->count != 0; };
  bool isAccident() { return this->count >= this->accident; };
  void reset(){ 
    debugPretty;
    //debugPrintln(__TIME__); 
    //this->nextMs+=this->periodMs;
    changeMs=millis();  
    this->count=0; };
  //void increase(){ this->count++; };
  uint operator++ (int) { 
    debugPretty;
    changeMs=millis(); 
    return ++this->count; };

  void tick(unsigned long incTimeoutMs=5*POLLING_TIME*3 ){
    if ( millis() - changeMs >= incTimeoutMs ){
      //count++;
      (*this)++;
    }

      // long unsigned nowMs=millis();
    //if ( this->nextMs== millis() ){ debugPretty; this->nextMs+=this->periodMs; ++this->count; }  
    if( ! this->isWrong() ) this->func(1000);
    else 
      if ( ! this->isAccident() ) this->func(200); 
      else { 
        debugPretty; 
        this->accidentFunc(); //ESP.restart();
      }
  };

} wrongCount(ledFlash, ESP.restart, 3 ) ;


class Relay {
public:
  Relay(uint port, uint8_t initStatus=LOW, uint closedPeriod=DEFAULT_CLOSED_SEC) : initStatus(initStatus)
  {
    this->port=port;
    this->closedPeriod=closedPeriod;
    this->status=initStatus;
    this->write();
    pinMode(port, OUTPUT);
    //digitalWrite(this->port, this->initStatus);
  };
  ~Relay(){};
  void tick(){
    if ( this->getStatus() ){
      time_t _now = time(nullptr);
      if ( changeStatusTime <= _now ){
        this->invert();
        // this->status = ! this->status;
        // //digitalWrite(this->port, this->status);
        // this->write();
        #if DEBUG_RELAY
        debugPrintf("Time for change %d\nRelay closed [%d].\n", _now,this->status);
        #endif
      }
    }
  };
  void open(uint period=0){
    if ( this->getStatus() ) return;
    time_t _now = time(nullptr);
    this->changeStatusTime = period != 0 ? (_now + period ) : (_now + this->closedPeriod); 
    this->invert();
    // this->status = ! this->initStatus;
    // this->write();
    #if DEBUG_RELAY
    debugPrintf("Relay open [%d].\nNow: %ld. Change time %ld\n", this->status, (uint)_now, this->changeStatusTime);
    #endif
  };

  uint getClosedPeriod(){
    return this->closedPeriod;
  };
  uint getStatus(){
    return this->status != this->initStatus;
  };

private:
  uint port;
  uint closedPeriod;
  uint8_t  status;
  const uint8_t initStatus;
  time_t changeStatusTime;
  void invert(){
    this->status = ! this->status;
    this->write();
  //  digitalWrite(this->port, this->status);
  };
  inline void write(){
     digitalWrite (this->port, this->status);
   };
};


#ifdef HW_622
/*
  struct HW622Relay {
    HW622Relay( const int port) : _port(port) {
      pinMode(this->_port,OUTPUT);  
    };
    int status(){ return digitalRead(this->_port); };
    private:
    const int _port;
  }; // relay(RELAY_PORT);
*/
class HW622 {
  public:
  HW622() : relay(Relay(RELAY_PORT,RELAY_INIT_STATUS)) {
    this->init();
  };
  
  // inline int  relayStatus(){ return digitalRead(this->_relay);};
  // inline void relaySet(int status){ digitalWrite(this->_relay, status); };
  // inline void relayTogle(){ this->relaySet( !this->relayStatus() ); };
  // inline void relayOn() { this->relaySet( ! RELAY_INIT_STATUS ); };
  // inline void relayOff(){ this->relaySet( RELAY_INIT_STATUS ); };

  //inline void relayTogle(){ digitalWrite(this->_relay, ! this->relayStatus()); };
  //inline void relayOn(){digitalWrite(this->_relay, ! RELAY_INIT_STATUS ); };
  //inline void relayOff(){digitalWrite(this->_relay, RELAY_INIT_STATUS ); };
  inline int  getInput(){ return digitalRead(this->_input);};
  private:
  Relay relay;
  //  const int _relay=RELAY_PORT;
  const int _input=INPUT_PORT;
  void init(){
    pinMode(this->_input, INPUT);
//    pinMode(this->_relay,OUTPUT);
//    this->relayOff();
  };
} hw_622;
#endif
