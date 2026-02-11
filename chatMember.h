#include "debug.h"
#include "wifiManager.h"
#include <cstdint>
#pragma once

#include "myFastBotClient.h"
//#include "fsSettings.h"
#include "newFsSettings.h"
// #include "old/expireButton.h"
// extern ExpireButton myButton;

#include "simpleButton.h"
extern SimpleButton myButton;

//extern SETTINGS::SettingsT settings;
extern BotSettings::Settings settings;

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
  static const char botNeedAdmin_MD[] PROGMEM = "*Назначьте бота Администратором канала\\!*";
};

void handleChatMember(fb::Update& u){
    static uint32_t waitAdmin = 0;

    long long fromId = u.message().from().id();
    long long chatId = u.message().chat().id();
    String chatTitle = u.message().chat().title().decodeUnicode();
    auto type = u.message().chat().type();

    fb::Message message;
    message.setModeMD();    
    message.chatID = u.message().from().id();
    debugPretty;
    debugPrintf("From:%lld\nchatId:%lld\tTitle:'%s'\n", fromId, chatId, chatTitle.c_str()); 
    auto entry = u.message().entry;
    //entry.stringify(Serial);
      //String cmd;
      //fb::InlineMenu menu;

      auto status = entry[tg_apih::new_chat_member][tg_apih::status];
      //auto status = entry.get("new_chat_member").get("status"); // value();
      debugPrintf("Status = %s\n", status.value().toString().c_str());

      switch (status.hash()) {

      // покидаем канал управления, переходим к диалогу с Админом
      case "left"_h :
        debugPrintln("Left chat");
        
        // удаляем канал
        menuIds.removeChannel(settings.getChatId(true) );

        message.chatID = settings.getAdminId();
        settings.set()->ChatId(0);
        
        if( settings.save() ){       
          //message.chatID = settings.getChatId();
          //message.setModeMD();
          message.text = BotChatTempl::deleteRecomends_MD;
          
          debugPrintln("New settings saved. Try create new keyboard.");

          auto res = myButton.creater( ); //settings.getChatId(), settings.getButton() );  

          if( res == SimpleButton::ReturnCode::ok ){
            debugPrintf("New keybord for %lld created\n", settings.getChatId());
          } else {
            debugPrintf("ERROR: create keybord for %lld\n", settings.getChatId());
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
        if ( settings.isAdmin(u.message().from().id() )) {
          debugPrintf("Switch by type chat: %s\n", 
            type == fb::ChatRead::Type::channel 
              ? "channel" 
              : type == fb::ChatRead::Type::privateChat ? "private chat" :
                "group"
          );

          // clean old channel/group
          //menuIds.removeChannel(settings.getChatId(true) );

          switch ( type ){
            case fb::ChatRead::Type::channel:
              debugPrintf("Set chat %lld as control channel. Make control keyboard\n", chatId);
              // clean old channel menuId if exists
              // if( settings.getChatId(true) ){

              //   menuIds.removeChannel(settings.getChatId(true) );
              // }

              if( settings.set()->ChatId( chatId ) ){ 
                
                menuIds.setChannelName( chatId, chatTitle );
                
                //String channelName =  channelName::addChannelName( chatId ); /// !== channelName::get()
                String channelName(CHANNEL_FOR_CONTROL);
                channelName += TelegramMD::asBold( TelegramMD::textIn_( chatTitle, '\'' ),  MARKDOWN_TG::escape);
                
                //LastMsg buttonInChannel( chatId );
              
                unsigned long prevChannelButton = menuIds.getMenuId( chatId ); //buttonInChannel.get();
                if( prevChannelButton != 0 ){
                  fb::Result res;
                  res = bot.deleteMessage( chatId, prevChannelButton );
                  if ( res.valid() && ! res.isError() ) {
                    debugPrintf("Button msgId=%lu in this channel %lld deleted\n", prevChannelButton, chatId );
                    //delay(300);

                  }
                }
                if ( settings.save() ){ 
                  
                  auto res = myButton.creater( ); //settings.getChatId(), settings.getButton() );  
                  unsigned long lastSendMs = millis();

                  if ( res == SimpleButton::ReturnCode::ok ) {
                    debugPrintln("New button created in channel");
                    
                  }

                  // сообщение админу 
                  if ( settings.hasAdmin() ){
                    
                    auto adminId = settings.getAdminId();
                    //LastMsg buttonMsg( adminId );
                    //uint msgId = lastMsg.get();

                    //channelName::load(settings.getChatId(true));
                    if ( lastSendMs ) 
                      while( millis()-lastSendMs < 300){
                        delay(1);
                      }
                    //if ( buttonMsg.get() == 0) {
                    if ( menuIds.getMenuId( adminId ) == 0 ){
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
                      message.messageID = menuIds.getMenuId( adminId ); //buttonMsg.get();
                      message.mode = fb::Message::Mode::MarkdownV2;
                      message.text += channelName; //::get();
                      bot.editText(message);
                    }
                  }
                }
              
            //channelName::freeMemory();
              }
              break;
            case fb::ChatRead::Type::group:
            case fb::ChatRead::Type::supergroup:

              if ( waitAdmin ) {
                bot.deleteMessage( fromId, waitAdmin);
                waitAdmin = 0;
              }
              debugPrintln("Для группы создаем клавиатуру с меню: Открыть");
              if( settings.set()->ChatId( chatId ) ){ 
                
                menuIds.setChannelName( chatId, chatTitle );
                // шлём команды и клавиатуру
                // {
                //   fb::MyCommands commands("open;help", "Открыть;Помощь");
                //   FB_LOG("set cmds");
                //   bot.setMyCommands(commands);
                // }
                {
                  fb::Message msg( F("*не удалять\\!\\!*"), chatId);
                  // msg.chatID = chatId;
                  // msg.text = F("*не удалять\\!\\!*");
                    fb::Menu menu;
                    menu.placeholder = settings.getButtonHeader();//(F("Кнопка меню"));
                    menu.addButton( settings.getButtonName() );//F("Открыть"));
                    menu.resize = true;
                    menu.persistent = true;
                    
                    msg.setMenu(menu);
                  msg.protect = true;
                  msg.notification = true;
                  msg.mode = fb::Message::Mode::MarkdownV2;

                  auto res = bot.sendMessage(msg, true );
                  if ( res.valid() && ! res.isError() ) {
                    auto msgId = bot.lastBotMessage();
                    menuIds.setMenuId( chatId, bot.lastBotMessage() );
                    bot.pinChatMessage( chatId, msgId );
                  }
                }
                //message.text ="ToDo: add create menu Open";
              }
              
              break;

          }
        }
        break;
      default:
        debugPrintln("Need add bot as administrator");
        message.text = BotChatTempl::botNeedAdmin_MD;
        auto res = bot.sendMessage(message, true);
        if( res.valid() && ! res.isError() ) waitAdmin = bot.lastBotMessage();
        message.text = "";
      }

  if( ! message.text.isEmpty() && (Text)message.chatID != 0ll ) {
    debugPrintf("message:%s\nto:%s\n", message.text.c_str(), ((Text)message.chatID).toString().c_str() ); 
    auto res = bot.sendMessage(message,true);
    res.printTo(Serial); debugPrintln();

  }
}