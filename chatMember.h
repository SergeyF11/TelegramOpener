#include "debug.h"
#include "wifiManager.h"
#include <cstdint>
#pragma once

#include <FastBot2.h>
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
      fb::InlineMenu menu;

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
        debugPrintf("Set chat %lld as control channel. Update or make control keyboard\n", chatId);
        {
        if( settingsNew.set()->ChatId( chatId ) )
          if ( settingsNew.save() ){ 

            channelName::save(chatId, chatTitle );
            channelName::freeMemory();
            debugPrintf("New settings saved. Try create update keyboard in channel '%s'[%lld].\n", chatTitle.c_str(), chatId);
            //myButton.updater( bot, chatId, settingsNew.getButton(), false );
            myButton.needUpdate(SimpleButton::NeedUpdate::setTrue);
            myButton.updater( chatId, settingsNew.getButton(), false); 
            
            //auto res = myButton.updater( chatId, settingsNew.getButton(), true );
            // debugPrintln( myButton.codeToString( res ));

            // if ( res != SimpleButton::ReturnCode::ok  ) {
            //   res = myButton.creater(bot, chatId, settingsNew.getButton());  
            //   debugPrintln( myButton.codeToString( res ));

            //   if( res == SimpleButton::ReturnCode::ok ){
            //     debugPrintln("New keybord created");
            //   }  
            //} 
            //if ( res < SimpleButton::ReturnCode::wrongResponse ){

              // удаляем кнопку если она есть у админа 
              {
                auto chat = settingsNew.getAdminId();
                LastMsg lastMsg(chat );
                uint msgId = lastMsg.get();
                channelName::load(settingsNew.getChatId(true));
                if ( msgId == 0) {
                  // нет кнопки - создаем сообщение
                  fb::Message message;
                  message.chatID = chat;
                  message.mode = fb::Message::Mode::MarkdownV2;;
 
                  //message.text += F("\nМой канал управления *");
                  message.text = CHANNEL_FOR_CONTROL;
                  message.text += F("*");
                  if ( channelName::isEmpty() ){
                    message.text += "\\#\\"; 
            //                                                    1001715239030ll
                    message.text += 1000000000000ll + settingsNew.getChatId(true);
                  } else { 
                    message.text += F("\\'");
                    message.text += channelName::get(); 
                    message.text += F("\\'");
                  }
                  message.text += F("*");
                  bot.sendMessage(message);
                } else {
                  // есть кнопка в чате админа => редактируем
                  fb::TextEdit message;
                  message.chatID = chat;
                  message.messageID = msgId;
                  message.mode = fb::Message::Mode::MarkdownV2;
                  //message.setModeMD();
                  message.text = CHANNEL_FOR_CONTROL;
                  message.text += F("*");
                  if ( channelName::isEmpty() ){
                    message.text += "\\#\\"; 
            //                                                    1001715239030ll
                    message.text += 1000000000000ll + settingsNew.getChatId(true);
                  } else { 
                    message.text += F("\\'");
                    message.text += channelName::get(); 
                    message.text += F("\\'");
                  }
                  message.text += F("*");
                  bot.editText(message);

                }
              }
/*             auto res = myButton.cleaner( settingsNew.getAdminId(), true); 

            debugPrintln( myButton.codeToString( res ));
            if( res < SimpleButton::ReturnCode::wrongResponse )
                debugPrintln("Admins channel keyboard deleted");
            } */
            //message.text = BotChatTempl::deleteRecomends_MD;
            //debugPrintln( myButton.codeToString( res ) );
          }
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