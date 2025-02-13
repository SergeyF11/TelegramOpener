#include "debug.h"
#pragma once

#include "myFastBotClient.h"
#include "env.h"
// #include "LastMsgInFile.h"
#include "newFsSettings.h"

#ifdef BUTTON_NAME
namespace ButtonInlineMenu
{
  // static const char * bCmds PROGMEM = "~o~";
  static const char *bCmds = strdup(String(ESP.getChipId(), HEX).c_str());
};
#else
#error BUTTON_NAME must be defined
#endif
#ifndef HEADER_STRING
#error HEADER_STRING must be defined
#endif

namespace Wait
{
  static unsigned long _startMs;
  // void start(){ _startMs = millis(); }
  void wait(unsigned long period = 0)
  {
    if (period)
    {
      while ((millis() - _startMs) < period)
      {
        delay(1);
      }
    }
    else
    {
      _startMs = millis();
    }
  };
};

class SimpleButton
{
private:
  BotSettings::Settings *sets;
  FastBot2Client *botP = nullptr;
  unsigned long lastUpdate;
  unsigned long _expiredPeriod;
  unsigned long _extraTime = 0;

  const long long chatId() const
  {
    return sets->getChatId();
  };
  const char *buttonName() const
  {
    return sets->getButtonName();
  };
  const char *buttonHeader() const
  {
    return sets->getButtonHeader();
  };
  const char *buttonReport() const
  {
    return sets->getButtonReport();
  };
  bool _needUpdate = false;

public:
  enum ReturnCode
  {
    ok = 0,
    notNeeded,
    isPolling,
    noChat,
    noMesgId,
    wrongResponse
  };
  // enum NeedUpdate {
  //   get = -1,
  //   setFalse = 0,
  //   setTrue = 1
  // };

  bool needUpdate(bool nd)
  {
    this->_needUpdate = nd;
    return _needUpdate;
  };
  bool needUpdate() const
  {
    return _needUpdate;
  };

#if defined debug_print
  const char *codeToString(ReturnCode code) const
  {

    switch (code)
    {
    case ReturnCode::ok:
      return PSTR("ok");
      break;
    case ReturnCode::notNeeded:
      return PSTR("notNeeded");
      break;
    case ReturnCode::isPolling:
      return PSTR("isPolling");
      break;
    case ReturnCode::noChat:
      return PSTR("noChat");
      break;
    case ReturnCode::noMesgId:
      return PSTR("noMesgId");
      break;
    case ReturnCode::wrongResponse:
      return PSTR("wrongResponse");
      break;
    }
    return PSTR("?");
  }
#endif

  SimpleButton() {};
  // SimpleButton(  FastBot2Client& bot, ,
  //     const unsigned long expiredPeriod){
  //   this->set( bot, chatId, button, expiredPeriod);
  // };
  // void set( FastBot2Client& bot, const long long chatId,
  //      BotSettings::ButtonT& button,
  //     const unsigned long expiredPeriod){
  //   //this->sets=&settings;
  //   this->chatId=chatId;
  //   this->_expiredPeriod = expiredPeriod;
  //   this->buttonP = &button;
  //   this->botP = &bot;
  // };

  /// @brief
  /// @param bot
  /// @param settings
  /// @param expiredPeriod
  SimpleButton(FastBot2Client &bot, BotSettings::Settings &settings, const unsigned long expiredPeriod)
  {
    this->set(bot, settings, expiredPeriod);
  };

  void set(FastBot2Client &bot, BotSettings::Settings &settings, const unsigned long expiredPeriod)
  {
    sets = &settings;
    ;
    _expiredPeriod = expiredPeriod;
    botP = &bot;
  };

  ~SimpleButton() {};
  // const ReturnCode cleaner(const long long chat, const bool waitBotResponse=false){
  //   if ( this->botP != nullptr )
  //     return cleaner( *this->botP, chat, waitBotResponse );
  //   // else
  //   return ReturnCode::wrongResponse;
  // };

  const ReturnCode cleaner(const bool waitBotResponse = false)
  {
    if (!menuIds.hasMenuId(this->chatId()))
      return ReturnCode::noMesgId;

    unsigned long keybId = menuIds.getMenuId(this->chatId());

    fb::Result res = botP->deleteMessage(this->chatId(), keybId, waitBotResponse);
    if (!waitBotResponse || res.valid())
    {
      menuIds.removeMenuId(this->chatId());
      return ReturnCode::ok;
    }
    return ReturnCode::wrongResponse;
  };

  // const ReturnCode cleaner( const bool waitBotResponse=false){
  //   if ( ! menuIds.hasMenuId( sets->getChatId() ) ) return ReturnCode::noMesgId;

  //   ReturnCode ret=ReturnCode::wrongResponse;
  //   fb::Result res;
  //   unsigned long keybId = menuIds.getMenuId( sets->getChatId() );

  //   res = botP->deleteMessage( sets->getChatId(), keybId, waitBotResponse );
  //   if ( ! waitBotResponse || res.valid() ) {
  //     menuIds.removeMenuId( sets->getChatId() );
  //     ret = ReturnCode::ok;
  //   }
  //   return ret;
  // };

  inline String dynamicCmd(const char *_cmd)
  {
    // return String(_cmd ) + millis();
    String cmd(_cmd);
    cmd += millis();
    return cmd;
  };

  void setExtraTime(const unsigned long extraTime)
  {
    _extraTime = extraTime;
  };

  bool isExpired(unsigned long bTime)
  {
    debugPrintf("isExpired()\n%lu > %lu\n", millis() - bTime, _expiredPeriod + _extraTime);
    return ((millis() - bTime) > (_expiredPeriod + _extraTime));
  };

  // CREATOR
  //  const ReturnCode creater( const long long chat, const BotSettings::ButtonT& button ){
  //    debugPretty;
  //    if ( chat == 0ll ) return ReturnCode::noChat;

  //   fb::InlineMenu menu( (const char *)button.name, dynamicCmd(ButtonInlineMenu::bCmds).c_str() );

  //   fb::Message message;

  //   message.chatID = chat;
  //   message.protect = true;
  //   message.text = button.header;
  //   message.setInlineMenu(menu);

  //   fb::Result res = botP->sendMessage(message, true);
  //   // int trys=3;
  //   // while( trys-- ){
  //   //   res = botP->sendMessage(message, true);
  //   //   if ( res.valid() ) break;
  //   //   debugPrintln("Not valid response. Try in 0.5 sec");
  //   //   delay(500);
  //   // }

  //   if ( ! res.valid()) {
  //     debugPrintf("Chat:'%lld'\nHeader:'%s'\nText:'%s', cmd:'%s'\n",
  //       chat, button.header , menu.text.c_str(), menu.data.c_str() );
  //     return ReturnCode::wrongResponse;
  //   }

  //   menuIds.setMenuId(chat, botP->lastBotMessage());

  //   this->lastUpdate=millis();

  //   this->needUpdate(NeedUpdate::setFalse);
  //   return ReturnCode::ok;
  // };

  /// @brief creator SimpleButton. Создаёт динамическую кнопку в чате settings.chatId для открытия реле
  /// @return error code ReturnCode
  const ReturnCode creater()
  {
    debugPretty;
    debugPrintf("chat=%lld, Button header=%s, name=%s, cmd=%s, wait\n",
                chatId(), buttonHeader(), buttonName(), dynamicCmd(ButtonInlineMenu::bCmds).c_str());
    ReturnCode ret;
    if (chatId() == 0)
      ret = ReturnCode::noChat;
    else
    {

      fb::Message message;

      fb::InlineMenu menu(buttonName(), dynamicCmd(ButtonInlineMenu::bCmds).c_str());
      message.chatID = chatId(); // sets->getChatId();
      message.protect = true;
      message.text = buttonHeader();
      message.setInlineMenu(menu);

      fb::Result res = botP->sendMessage(message, true);

      if (!res.valid())
      {
        /*       debugPrintf("Chat:'%lld'\nHeader:'%s'\nText:'%s', cmd:'%s'\n",
                chatId(), buttonHeader(), menu.text.c_str(), menu.data.c_str() ); */
        ret = ReturnCode::wrongResponse;
      }
      else
      {
        ret = ReturnCode::ok;
        menuIds.setMenuId(chatId(), botP->lastBotMessage());
        this->lastUpdate = millis();
        this->needUpdate(false);
        // debugPrintf("\tmsg=%lu\n", (unsigned long)menuIds.getMenuId( chatId() ) );
      }
    }
    debugPrintf("\tResp=%s\n", codeToString(ret));

    return ret;
  };

  enum CodeButtonE
  {
    ButtonName,
    ButtonReport,
  };

const ReturnCode updater(const bool waitBotResponse = false, const int codeButton = CodeButtonE::ButtonName)
  {
    const char *buttonTxt = (codeButton == ButtonName) ? buttonName() : buttonReport();
    debugPretty;
    debugPrintf("chat=%lld, msg=%lu, Button header=%s, name=%s, cmd=%s, wait=%s\n",
                chatId(), (unsigned long)menuIds.getMenuId(chatId()), buttonHeader(), buttonTxt, dynamicCmd(ButtonInlineMenu::bCmds).c_str(),
                waitBotResponse ? "true" : "false");
    ReturnCode ret;
    if (chatId() == 0)
      ret = ReturnCode::noChat;
    else if (!menuIds.hasMenuId(chatId()))
      ret = ReturnCode::noMesgId;
    else
    {
      // fb::Result res;
      String cmd = dynamicCmd(ButtonInlineMenu::bCmds);
      fb::TextEdit text;
      text.text = buttonHeader(); // sets->getButtonHeader();
      text.chatID = chatId();
      text.messageID = menuIds.getMenuId(chatId());

      fb::InlineMenu menu(buttonTxt, cmd.c_str());
      text.setInlineMenu(menu);

      auto res = botP->editText(text, waitBotResponse);
      if (waitBotResponse && !res.valid())
      {
        this->needUpdate(true);
        ret = ReturnCode::wrongResponse;
      }
      else
      {
        // debugPrintln( res.c_str() );
        this->lastUpdate = millis();
        this->needUpdate(false);
        ret = ReturnCode::ok;
      }
    }
    debugPrintf("\tResp=%s\n", codeToString(ret));
    return ret;
  };

  
  void tick()
  {
    // debugPretty;

    if (this->needUpdate() || millis() - this->lastUpdate >= _expiredPeriod)
    {
      debugPrintf("Button tick. Need:%d\tlastUpdate:%ld\tperiod:%ld\n", this->needUpdate(), this->lastUpdate, _expiredPeriod);

      this->updater();
    }
  };
};
