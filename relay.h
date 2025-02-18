#include <cstdint>
//#include "twi.h"
#include <assert.h>

// #include "core_esp8266_features.h"
// #include "common.h"
#pragma once
//#include <time.h>
#ifdef TEST_WEMOS
#define RELAY_PORT LED_BUILTIN
#define RELAY_INIT_STATUS HIGH
("Using LED_BUILTIN for relay imitation. For test only")
#endif
#ifdef HW_622
#define RELAY_PORT 4
#define RELAY_INIT_STATUS LOW
#define INPUT_PORT 5
#endif

#ifndef RELAY_PORT
#error Must be defined BOARD as TEST_WEMOS or HW_622 for define RELAY_PORT
#else

#pragma message("Using BOARD settings")
#endif

#define DEFAULT_OPEN_SEC 3
#define MAX_OPEN_SECS 60*60*24 // one day
//#define LOW 0
//#define HIGH 1
#define DEBUG_RELAY 0

// #define BULITIN_LED_MACRO(period, flash, init) { \
//   static int initState, pinVal = init;     \
//   static unsigned long t = 0UL;     \
//   unsigned long now; now=millis();       \
//   uint p = (pinVal!=initState) ? (uint)(period-flash) : (uint)(flash); \
//   if( now - t >= p ) { /*pinVal==initState ? (unsigned long)(period) : (unsigned long)(flash) ) {   */     \
//     if( Serial ) Serial.printf("Change pinVal %d for %ld\n",pinVal,now); \
//     pinVal = !pinVal; \
//     digitalWrite(BUILTIN_LED, pinVal);                     \
//     t = now;  }                \
// }


class OutputPin {
  public:
  enum State {
    OFF,
    ON,
  };
  private:
  const uint8_t pin;
  const uint8_t initState;
  uint8_t pinValue;
  public:
  inline void invert(){
    pinValue = !pinValue;
    write();
  }
  inline void write() const {
    digitalWrite(pin,pinValue);
  };
  inline void write(State val){
    pinValue = ( val == State::OFF ) ? initState : !initState;
    write();
  };
  inline void off(){ pinValue = initState; write(); };
  inline void on(){ pinValue = !initState; write(); };
  inline State state(bool real=false) const { 
    return real ? 
      (State)(digitalRead(pin) != initState ) : 
      (State)(pinValue != initState); 
  };
  OutputPin(const uint8_t pin, const bool stateOn=HIGH) :
    pin(pin), initState(State(!stateOn))
  {
    pinMode(pin,OUTPUT);
    off();
  }

};

// defined led instatnce with on(), off(), toggle(), flash(period,flashMs) functions
class Led : public OutputPin {  
  private:
//  OutputPin pin;
  unsigned long changeStateMs=0;
  bool flashed = true;
  public:

  //define led on pin and ON state value
  Led(const uint8_t pin, const bool stateOn=HIGH) :
    OutputPin(pin, stateOn)
  { };

  // Flash led on flashMs with periodMs. Call in loop
  // periodMs must be =0 or greater flashMs
  void flash(const uint periodMs=1000,const uint flashMs=1) {
    #ifdef debug_print
    assert( ( /*periodMs == 0 ||*/ periodMs > flashMs ) && "Error: periodMs must be = 0 or > flashMs" );
    //assert((void("void helps to avoid 'unused value' warning"), periodMs > flashMs ));
    #endif
    if ( ! flashed ) return;
    unsigned long currentMs = millis(); 
    //if ( periodMs ) 
      if( state() == OutputPin::OFF ){
        if( currentMs - changeStateMs < (periodMs-flashMs) ) return;
        else on();
      } else {
        if( currentMs - changeStateMs < flashMs ) return;
        else off();
      }
    //else off();
    changeStateMs = currentMs;
  };

  // turn off flashing in loop
  void flashOff(){ off(); flashed = false; };

  //turn On flashing in loop
  void flashOn(){ flashed = true; };
  
  // invert led state now
  inline void toggle(){
    changeStateMs = millis();
    invert();
  };

  //invert led state after MS. Need call the func in loop
  void toggle(int period) {
    //unsigned long currentMs = millis(); 
    if ( period ){
      if ( millis() - changeStateMs < period ) return;
    }
    toggle();
    };
} builtInLed(LED_BUILTIN, LOW);

/// @brief Relay может срабатывать на определённое время openPeriod.
///  Call Relay::tick() in loop for correct works
/// @note Set openPeriod to 0 for open/close manual control
class Relay : OutputPin {  
  private:
  //OutputPin pin;
  unsigned long openPeriodMs; 
  unsigned long changeStateMs=0;

  public:

  void setOpenPeriod(const unsigned long sec = DEFAULT_OPEN_SEC) { 
    openPeriodMs = 1000 * ( (sec<=MAX_OPEN_SECS) ? sec : MAX_OPEN_SECS );
    debugPrintf("Relay open period=%dms\n", openPeriodMs );
  };
  // define relay instance 
  // args: pin, init state, period for open state 
  Relay(const uint port, const uint8_t initState=LOW, const uint openPeriod=DEFAULT_OPEN_SEC) :
    OutputPin(port, !initState ) 
    { setOpenPeriod(openPeriod); };
    
  inline bool isAutocloseable(){ return openPeriodMs != 0; };

  unsigned long getOpenPeriod() const { return openPeriodMs/1000; };
  bool isOpen(){ return state(true) == State::ON ; };

  // Call in loop for change state the relay
  void tick(){
    if ( isAutocloseable() ) {
      if ( state() == OutputPin::ON  ){
        //time_t _now = time(nullptr);
        if ( millis() - changeStateMs >= openPeriodMs ){
          off();
          #ifdef TEST_WEMOS
          builtInLed.flashOn();
          #endif
        }
      }
    }
  };
  // Off relay for close
  void close(){
    if ( state() == OutputPin::OFF ) return;
    changeStateMs = millis(); //period != 0 ? (_now + period ) : (_now + this->openPeriod); 
    #ifdef TEST_WEMOS
    builtInLed.flashOn();
    #endif  
    off();
  };
  // On relay for open device
  void open(){
    if ( state() == OutputPin::ON ) return;
    changeStateMs = millis(); //period != 0 ? (_now + period ) : (_now + this->openPeriod); 
    #ifdef TEST_WEMOS
    builtInLed.flashOff();
    #endif  
    on();
  };

};




struct WrongCount {

  private:
  uint count=0;
  unsigned long changeMs =0; 
  const uint accident;
  unsigned long wrongPeriod;
  void (*okFunc)();
  void (*wrongFunc)();
  public:
    void (*accidentFunc)(void);
  //WrongCount(){ };
  WrongCount(void (*okF)(), void (*wrongF)(), void (*accidentF)(), const uint accident=3, const uint wrongPeriod=1000 ) :
    okFunc(okF), wrongFunc(wrongF), accidentFunc(accidentF), accident(accident), wrongPeriod(wrongPeriod)
   { };

  uint get() const {return count; };
  operator uint() const { return count; };

  bool isWrong(){ return count != 0; };
  bool isAccident() { return count >= accident; };
  void reset(){ 
    debugPretty;
    //debugPrintln(__TIME__); 
    //this->nextMs+=this->periodMs;
    changeMs=millis();  
    count=0; 
  };
  //void increase(){ this->count++; };
  uint operator++ (int) { 
    debugPretty;
    changeMs=millis(); 
    return ++count; 
  };
  void setWrongPeriod(unsigned long ms){ wrongPeriod = ms; }; 
  void tick(/* unsigned long incTimeoutMs=5*POLLING_TIME*3 */ ){
    if ( millis() - changeMs >= wrongPeriod ){
      //count++;
      (*this)++;
    }
    if( ! isWrong() ) okFunc();
    else if ( ! isAccident() ) wrongFunc(); 
      else { 
        debugPretty; 
        accidentFunc(); 
      }
  };

} //  wrongCount( [](uint p){ builtInLed.flash(p); }, ESP.restart, 3, POLLING_TIME/1000*5 ) ;
 wrongCount(
  [](){ builtInLed.flash(1024); },
  [](){ builtInLed.flash(300); },
  ESP.restart, 
  3, 1000 ) ;

/// @brief обработчик сырого ответа апдейтера Телеграма. Используетеся для сброса счетчика wrongCount
/// @param resp 
void rawResponse(const su::Text& resp){
  //debugPrint(__TIME__ ); debugPrint(' ');
  //debugPretty;   // debugPrintln(resp.c_str());
  if ( resp.valid()) wrongCount.reset(); 
  else {
    wrongCount++;
    debugPretty;
  }
};

// class RelayOld {
// public:
//   RelayOld(uint port, uint8_t initState=LOW, uint openPeriod=DEFAULT_OPEN_SEC) : initState(initState)
//   {
//     this->port=port;
//     this->openPeriod=openPeriod;
//     this->pinVal=initState;
//     this->write();
//     pinMode(port, OUTPUT);
//     //digitalWrite(this->port, this->initState);
//   };
//   ~RelayOld(){};
//   void tick(){
//     if ( this->status() ){
//       time_t _now = time(nullptr);
//       if ( changeStatusTime <= _now ){
//         this->toggle();
//         // this->pinVal = ! this->pinVal;
//         // //digitalWrite(this->port, this->pinVal);
//         // this->write();
//         #if DEBUG_RELAY
//         debugPrintf("Time for change %d\nRelay closed [%d].\n", _now,this->pinVal);
//         #endif
//       }
//     }
//   };
//   void on(uint period=0){
//     if ( this->status() ) return;
//     time_t _now = time(nullptr);
//     this->changeStatusTime = period != 0 ? (_now + period ) : (_now + this->openPeriod); 
//     this->toggle();
//     // this->pinVal = ! this->initState;
//     // this->write();
//     #if DEBUG_RELAY
//     debugPrintf("Relay on [%d].\nNow: %ld. Change time %ld\n", this->pinVal, (uint)_now, this->changeStatusTime);
//     #endif
//   };

//   uint getClosedPeriod(){
//     return this->openPeriod;
//   };
//   uint status(){
//     return this->pinVal != this->initState;
//   };

// private:
//   uint port;
//   uint openPeriod;
//   uint8_t  pinVal;
//   const uint8_t initState;
//   time_t changeStatusTime;
//   void toggle(){
//     this->pinVal = ! this->pinVal;
//     this->write();
//   //  digitalWrite(this->port, this->pinVal);
//   };
//   inline void write(){
//      digitalWrite (this->port, this->pinVal);
//    };
// };


#ifdef HW_622
/*
  struct HW622Relay {
    HW622Relay( const int port) : _port(port) {
      pinMode(this->_port,OUTPUT);  
    };
    int pinVal(){ return digitalRead(this->_port); };
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
  // inline void relaySet(int pinVal){ digitalWrite(this->_relay, pinVal); };
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
