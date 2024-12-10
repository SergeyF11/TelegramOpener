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
  //su::Text firstName; // = e.get("first_name");
  // e.printTo(Serial);
  // e["first_name"].printTo(Serial);
  // e["username"].printTo(Serial);

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

namespace BotChat {
  //struct TemplateT {
  static const char leftChat_MD_Tmpl[] PROGMEM = "Бот `%s` удалён из чата `%s`\\. ";
  //};
  static const char deleteRecomends_MD[] PROGMEM = "_Рекомендую удалить меню управления ботом в чате\\._";
}

void handleChatMember(fb::Update& u){
    long long from = u.message().from().id();
    long long chatId = u.message().chat().id();
    String chatTitle = u.message().chat().title();
    fb::Message message;
    message.setModeMD();    
    message.chatID = u.message().from().id();
    debugPrintf("From:%lld\n", from); 
    auto entry = u.message().entry;
    //entry.stringify(Serial);
      String cmd;
      fb::InlineMenu menu;

      auto status =entry[tg_apih::new_chat_member][tg_apih::status];
      //auto status = entry.get("new_chat_member").get("status"); // value();
      debugPrintf("Status = %s\n", status.value().toString().c_str());

      switch (status.hash()) {


      case "left"_h :
        debugPrintln("Left chat");
        {

        settingsNew.set()->ChatId(0);
        
        if( settingsNew.save() ){       
          debugPretty;

          auto res = myButton.creater( settingsNew.getAdminId(), settingsNew.getButton() );  
          if( res == SimpleButton::ReturnCode::ok ){
            debugPrintln("Create new keybord for admin");
          } 
          debugPrintln( myButton.codeToString( res ) );
          
        } else {
          message.text += "*";
          message.text += BotSettings::NotSaveStr;
          message.text += "*";
        }
        message.chatID = settingsNew.getAdminId();
        
        }
        break;
      case "administrator"_h :
        debugPrint("Update or make control keyboard in chat ");
        debugPrintln( chatId);
        {
        if( settingsNew.set()->ChatId( chatId ) )
          if ( settingsNew.save() ){ 

            channelName::save(chatId, chatTitle );
            channelName::freeMemory();

            //myButton.needUpdate(true);
            debugPretty;
            //myButton.updater( bot, chatId, settingsNew.getButton(), false );
            myButton.needUpdate(SimpleButton::NeedUpdate::setTrue);

            //auto res = myButton.updater( bot, chatId, settingsNew.getButton(), true );
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
            auto res = myButton.cleaner( settingsNew.getAdminId(), true); 
            debugPrintln( myButton.codeToString( res ));
            if( res < SimpleButton::ReturnCode::wrongResponse )
                debugPrintln("Admin keyboard deleted");
            }
            //debugPrintln( myButton.codeToString( res ) );
            //}
        }
        break;
      default:
        debugPrintln("Need add bot as administrator");
      }

  debugPrint("message:"); 
  debugPrintln(message.text);
  debugPrint("to:"); debugPrintln(message.chatID);

  if( ! message.text.isEmpty() ) {
    auto res = bot.sendMessage(message,true);
    res.printTo(Serial); debugPrintln();

  }
}