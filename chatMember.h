#include "debug.h"
#include "wifiManager.h"
#include <cstdint>
#pragma once

#include <FastBot2s.h>
//#include "fsSettings.h"
#include "newFsSettings.h"
// #include "old/expireButton.h"
// extern ExpireButton myButton;

#include "simpleButton.h"
extern SimpleButton myButton;

//extern SETTINGS::SettingsT settings;
extern BotSettings::Settings settingsNew;

void getNameFromEntry(String& txt, gson::Entry e, const char* prefix=((char *) 0), const char* postfix=((char *)0) ){
  txt += prefix;

  if ( e.has("first_name") ) txt += e["first_name"].value().toString(); //firstName.decodeUnicode();
  else if ( e.has( "username") ) txt += e["username"].value().toString() ;
  else { 
    txt += F("Незнакомец с id#");
    txt += e["id"].toString();
  }
  txt += postfix;
};
inline void getNameFromEntry(String& txt, gson::Entry e, const String& prefix, const String& postfix ){
  getNameFromEntry(txt, e, prefix.c_str(), postfix.c_str());
};

namespace fb_adds {
  bool isMyChatMember( fb::Update& u) {
    return  u.type() == fb::Update::Type::MyChatMember;
  };
}

namespace BotChatTempl {
  static const char leftChat_MD_Tmpl[] PROGMEM = "Бот `%s` удалён из чата `%s`\\. ";
  static const char deleteRecomends_MD[] PROGMEM = "_Рекомендую удалить меню управления ботом в чате\\._";
  static const char botNeedAdmin_MD[] PROGMEM = "*Бот должен быть Администратором канала\\!*";
};

void handleChatMember(fb::Update& u){
    long long from = u.message().from().id();
    long long chatId = u.message().chat().id();
    String chatTitle = u.message().chat().title().decodeUnicode();
    fb::Message message;
    message.setModeMD();    
    message.chatID = u.message().from().id();
    debugPretty;
    debugPrintf("From:%lld\nchatId:%lld\tTitle:'%s'\n", from, chatId, chatTitle.c_str()); 
    auto entry = u.message().entry;
    //entry.stringify(Serial);
      String cmd;
      //fb::InlineMenu menu;

      auto status =entry[tg_apih::new_chat_member][tg_apih::status];
      //auto status = entry.get("new_chat_member").get("status"); // value();
      debugPrintf("Status = %s\n", status.value().toString().c_str());

      switch (status.hash()) {

      // покидаем канал управления, переходим к диалогу с Админом
      case "left"_h :
        debugPrintln("Left chat");
        
        message.chatID = settingsNew.getAdminId();
        settingsNew.set()->ChatId(0);
        
        if( settingsNew.save() ){       
          //message.chatID = settingsNew.getChatId();
          //message.setModeMD();
          message.text = BotChatTempl::deleteRecomends_MD;
          
          debugPrintln("New settings saved. Try create new keyboard.");

          auto res = myButton.creater( settingsNew.getChatId(), settingsNew.getButton() );  

          if( res == SimpleButton::ReturnCode::ok ){
            debugPrintf("New keybord for %lld created\n", settingsNew.getChatId());
          } else {
            debugPrintf("ERROR: create keybord for %lld\n", settingsNew.getChatId());
             debugPrintln( myButton.codeToString( res ) );
          }

        } else {
          debugPrintln("ERROR: settings save.");
          message.text = "*";
          message.text += BotSettings::NotSaveStr;
          message.text += "*";
          // bot.sendMessage(message,false);

          // message.text = "";
        }

        break;

      case "administrator"_h :
        debugPrintf("Set chat %lld as control channel. Make control keyboard\n", chatId);
      if ( settingsNew.isAdmin(u.message().from().id() )) {
        if( settingsNew.set()->ChatId( chatId ) &&
          channelName::save(chatId, chatTitle )) {
            String channelName =  channelName::addChannelName( chatId ); /// !== channelName::get()
            LastMsg buttonInChannel( chatId );
            auto prevChannelButton = buttonInChannel.get();
            if( prevChannelButton != 0 ){
              fb::Result res;
              res = bot.deleteMessage( chatId, prevChannelButton );
              if ( res.valid() ) {
                debugPrintf("Button msgId=%lu in this channel %lld deleted\n", prevChannelButton, chatId );
                delay(300);

              }
            }
            if ( settingsNew.save() ){ 
              
              auto res = myButton.creater( chatId, settingsNew.getButton() );
              unsigned long lastSendMs = millis();

              if ( res == SimpleButton::ReturnCode::ok ) {
                debugPrintln("New button created in channel");
                
              }

              // сообщение админу 
              if ( settingsNew.hasAdmin() ){
                
                auto adminId = settingsNew.getAdminId();
                LastMsg buttonMsg( adminId );
                //uint msgId = lastMsg.get();

                //channelName::load(settingsNew.getChatId(true));
                if ( lastSendMs ) 
                  while( millis()-lastSendMs < 300){
                    delay(1);
                    }
                if ( buttonMsg.get() == 0) {
                  // нет кнопки - создаем сообщение
                  fb::Message newMsg;
                  newMsg.chatID = adminId;
                  newMsg.mode = fb::Message::Mode::MarkdownV2;
                  newMsg.text += channelName; //::get();
                  bot.sendMessage(newMsg);
                } else {
                  // есть кнопка в чате админа => подменяем на инфо о канале
                  fb::TextEdit message;
                  message.chatID = adminId;
                  message.messageID = buttonMsg.get();
                  message.mode = fb::Message::Mode::MarkdownV2;
                  message.text += channelName; //::get();
                  bot.editText(message);
                }
              }
            }
          }
          channelName::freeMemory();
        }
        break;
      default:
        debugPrintln("Need add bot as administrator");
        message.text = BotChatTempl::botNeedAdmin_MD;
      }

  if( ! message.text.isEmpty() && message.chatID.toInt64() != 0ll ) {
    debugPrintf("message:%s\nto:%lld\n", message.text.c_str(), message.chatID.toInt()); 
    auto res = bot.sendMessage(message,true);
    res.printTo(Serial); debugPrintln();

  }
}