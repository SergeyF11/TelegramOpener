#include "debug.h"
#pragma once

#include <FastBot2s.h>
#include "env.h"
//#include "LastMsgInFile.h"
#include "newFsSettings.h"

#ifdef BUTTON_NAME
namespace ButtonInlimeMenu {
  const char * bCmds PROGMEM = "~o~";
};
#else
#error BUTTON_NAME must be defined
#endif
#ifndef HEADER_STRING
#error HEADER_STRING must be defined
#endif


namespace Wait {
  static unsigned long _startMs; 
  //void start(){ _startMs = millis(); }
  void wait(unsigned long period=0){
    if ( period ){
      while( (millis() - _startMs) < period ){
        delay(1);
      }
    } else {
      _startMs = millis();
    }
  };
};

class SimpleButton  {
  public:
  enum ReturnCode { 
    ok = 0, notNeeded, isPolling, noChat, noMesgId, wrongResponse
  };
  enum NeedUpdate {
    get = -1,
    setFalse = 0,
    setTrue = 1
  };
  
  #if defined debug_print 
  String codeToString(ReturnCode code) const { 
    String out;
    switch (code){
    case ReturnCode::ok: out += F("ok");
      break;
    case ReturnCode::notNeeded: out += F("notNeeded");
      break; 
    case ReturnCode::isPolling: out += F("isPolling");
      break;
    case ReturnCode::noChat: out += F("noChat");
      break;
    case ReturnCode::noMesgId: out += F("noMesgId");
      break;
    case ReturnCode::wrongResponse: out += F("wrongResponse");
      break;
    }
    return out;
  }
  #endif
  
  SimpleButton()
  {  };
  SimpleButton(FastBot2& bot, unsigned long expiredPeriod){
    this->set( bot, expiredPeriod);
  };
  void set(FastBot2& bot, unsigned long expiredPeriod){
    this->_expiredPeriod = expiredPeriod;
    this->botP = &bot;
  };
  ~SimpleButton(){};
// const ReturnCode cleaner(const long long chat, const bool waitBotResponse=false){
//   if ( this->botP != nullptr ) 
//     return cleaner( *this->botP, chat, waitBotResponse );
//   // else
//   return ReturnCode::wrongResponse;
// };

const ReturnCode cleaner( const long long chat, const bool waitBotResponse=false){
  fb::Result res;
  //LastMsg keybId( chat );
  //if( keybId.get() == 0 ) return ReturnCode::noMesgId;
  if ( ! menuIds.hasMenuId( chat ) ) return ReturnCode::noMesgId;

  unsigned long keybId = menuIds.getMenuId( chat );
  //if( keybId == 0 ) return ReturnCode::noMesgId;
  
  res = botP->deleteMessage( chat, keybId, waitBotResponse );  
  if ( ! waitBotResponse || res.valid() ) { 
    //keybId.clean();
    menuIds.removeMenuId( chat);
    return ReturnCode::ok;
  }
  return ReturnCode::wrongResponse;
};

  inline   String dynamicCmd(const char * _cmd){
    //return String(_cmd ) + millis();
     String cmd;
     cmd += _cmd;
     cmd += millis();
     return cmd;
  };

  void setExpiredDelta( const unsigned long delta){
    _delta = delta;
  };

  bool isExpired(unsigned long bTime){
    debugPrintf("isExpired()\n%lu > %lu\n", millis() - bTime, _expiredPeriod + _delta);
    return ( millis() - bTime > _expiredPeriod + _delta );
  };

//CREATOR
const ReturnCode creater( const long long chat, const BotSettings::ButtonT& button ){ 
  debugPretty;
  if ( chat == 0ll ) return ReturnCode::noChat;

  fb::InlineMenu menu( (const char *)button.name, dynamicCmd(ButtonInlimeMenu::bCmds).c_str() ); 

  fb::Message message;
  
  message.chatID = chat;
  message.protect = true;
  message.text = button.header;
  message.setInlineMenu(menu);
  

  fb::Result res = botP->sendMessage(message, true);     
  // auto lastSend = millis();
  Wait::wait();
  //WaitC w;
  if ( ! res.valid() ){ //checkOk(res, false /*true*/) ) {
    debugPrintln("Not valid response. Try in 0.5 sec");
    // while( millis() - lastSend  <500 ){
    //   delay(1);
    // }
    Wait::wait(500);
    //w.wait(500);
    res = botP->sendMessage(message, true);
  }
  if ( ! res.valid()) {
    debugPrint("Not valid response: ");
    #ifdef debug_print
      res.getRaw().printTo(Serial);
    #endif
    debugPrintf("Chat:'%lld'\nHeader:'%s'\nText:'%s', cmd:'%s'\n",
      chat, button.header , menu.text.c_str(), menu.data.c_str() );
    return ReturnCode::wrongResponse;
  }
  
  // LastMsg lastMsg( chat );
  // lastMsg.set(botP->lastBotMessage());
  menuIds.setMenuId(chat, botP->lastBotMessage());

  this->lastUpdate=millis();
  this->needUpdate(NeedUpdate::setFalse);
  return ReturnCode::ok;
  };

  const ReturnCode updater( const long long chat, const BotSettings::ButtonT& button, const bool waitBotResponse=false){
    //if ( ! this->needUpdate() ) return ReturnCode::notNeeded;

    debugPretty; //Serial.println(__PRETTY_FUNCTION__);
    if ( chat == 0ll ) return ReturnCode::noChat;

    //if( ! bot.isPolling() ) return ReturnCode::isPolling;
    
    // если нет сохраненного id сообщения с меню, 
    //LastMsg lastMsg(chat );
    //uint msgId = lastMsg.get();

    if ( ! menuIds.hasMenuId( chat ) ) return ReturnCode::noMesgId; //lastMsg.set(bot.lastBotMessage());

    uint msgId = menuIds.getMenuId( chat );
    debugPrintf( "Chat id: %lld \tMsg id:%d\n", chat, msgId);
    
    String myMenuCmd = dynamicCmd(ButtonInlimeMenu::bCmds );
    // myMenuCmd += ButtonInlimeMenu::bCmds;
    // myMenuCmd += millis(); // time(nullptr); // добавляем время, чтобы отредактировать

    fb::InlineMenu menu( (const char *)button.name, myMenuCmd.c_str() );  
    debugPrint("Menu="); debugPrint( button.name ); debugPrint("|");debugPrintln(myMenuCmd.c_str());
       
      debugPrintf("Edit menu msg ID=%d\n", msgId );
      
      fb::TextEdit text;
      text.text = button.header;
      text.chatID = chat;
      text.messageID = msgId;

      text.setInlineMenu(menu);
debugPrint("fb::text.text="); debugPrintln(text.text);
debugPrint("fb::text.chatID="); debugPrintln(text.chatID);
debugPrint("fb::text.messageID="); debugPrintln(text.messageID);
      
      if ( waitBotResponse ){
        fb::Result res;
        res = botP->editText( text, waitBotResponse /*true*/ );

        if( ! res.valid()) {
          this->needUpdate(NeedUpdate::setTrue);

          #ifdef debug_print
            res.stringify(Serial);
            debugPrint("Not valid response: ");
            res.getRaw().printTo(Serial);
            debugPrintln();
          #endif
          //wrongCount++;
          return ReturnCode::wrongResponse;
        } //else wrongCount.reset(); //wrongCount=0;
      
      } else {
        botP->editText( text, waitBotResponse /*false*/ );

        //return ReturnCode::ok;
      }
    this->lastUpdate = millis();
    this->needUpdate(NeedUpdate::setFalse);
    return ReturnCode::ok;
  };
  //void needUpdate(bool nd=true) { this->_needUpdate=nd; };

  bool needUpdate(NeedUpdate set=NeedUpdate::get) const { 
    static bool _needUpdate = false;

    if( set != NeedUpdate::get )
      _needUpdate = (bool)set;
    return _needUpdate;
  };
  bool needUpdate(bool set) const { 
    return needUpdate( set ? NeedUpdate::setTrue : NeedUpdate::setFalse);
  };

  void tick( BotSettings::Settings& sets ){
    //debugPretty;
    
    if ( this->needUpdate() || millis() - this->lastUpdate >= _expiredPeriod) {//this->needUpdate( SimpleButton::NeedUpdate::setTrue );
    debugPrintf("Button tick. Need:%d\tlastUpdate:%ld\tperiod:%ld\n", this->needUpdate(), this->lastUpdate, _expiredPeriod);
      this->updater( sets.getChatId(), sets.getButton());
    }
  };

 private:

  FastBot2 * botP = nullptr;
  unsigned long lastUpdate;
  unsigned long _expiredPeriod;
  unsigned long _delta = 0; 
};

