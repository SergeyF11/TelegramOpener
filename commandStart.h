#include "relay.h"
#define THIS_BOT_LINK "https://t.me/rtu5024_bot"

#include "core/types/Message.h"

#pragma once
#define NUL_STR ((char*)0)

#include <FastBot2.h>
//#include "fsSettings.h"
#include "newFsSettings.h"

/*
struct TakeAdminT {
  long msgId;
  long long userId;
} takeAdmin;
*/
static constexpr char this_bot_link[] PROGMEM = THIS_BOT_LINK; //"t.me/rtu5024_bot";
static constexpr char noAdmin_MD[] PROGMEM = "*У меня пока нет хозяина\\. Хочешь им стать\\?*";
static constexpr char takeAdminStr[] PROGMEM = "Стать администратором";
static constexpr char haveAdmin_MD[] PROGMEM = "*У меня уже есть хозяин\\!*\n_Но ты всегда можешь завести [своего бота](" THIS_BOT_LINK ")_";
static constexpr char haveAdmin_Alert[] PROGMEM = "У меня уже есть хозяин!\nНо ты всегда можешь завести своего бота ";
static constexpr char youAdminAlready_MD[] PROGMEM = "*Вы уже являетесь администратором\\.*\n_Для помощи отправь_ `/help`";

//extern SETTINGS::SettingsT settings;
extern BotSettings::Settings settingsNew;

//обработка команды /start
void handleStart(fb::Update& u, fb::Message& message) {
  // будем беседовать с отправителем
  //message.chatID = u.message().from().id();

  if (u.message().from().id() == settingsNew.getAdminId() ){ //.admin) {
    String user(u.message().from().username());
    debugPrintf("Admin '%s'[#%s] start command\n", user,
                  (String)(u.message().from().id()));
    {
      fb::MyCommands commands("help;settings;startPortal", "Помощь;Настройки;Запустить CaptivеPortal");
      auto res = bot.setMyCommands(commands);
      if( res.valid() ) wrongCount.reset();
      else wrongCount++;
      // res.getRaw().printTo(Serial);
    }
    message.text = youAdminAlready_MD; // F("*Вы уже являетесь администратором\\.*\n_Для помощи отправь_ `/help`");
    
    // auto res = bot.sendMessage(message);
    // res.getRaw().printTo(Serial);

    // // уже отослано. не надо посылать
    // message.text = ((char *)0);

  } else {
    if ( settingsNew.getAdminId() ){ //settings.admin) {
      String user(u.message().from().username());
      debugPrintf("Unknown user '%s'[#%s] start command\n",
                    user,
                    (String)(u.message().from().id()));
      message.text = haveAdmin_MD; //F("*У меня уже есть хозяин\\!*\n_Но ты всегда можешь завести [своего бота](" THIS_BOT_LINK ")_");

    } else {
      String user(u.message().from().username());
      debugPrintf("New user '%s'[#%s] start command\n",
                    user,
                    (String)(u.message().from().id()));
      //debugPrintln("Registri new admin?");
      String cmd = "ta~";
      //cmd += bot.lastBotMessage()+1;
      
      fb::InlineMenu menu(takeAdminStr, cmd.c_str());
      message.text = noAdmin_MD; //F("*У меня пока нет хозяина\\. Хочешь им стать\\?*");
      message.setInlineMenu(menu);
      bot.sendMessage(message);

      // и запоминаем id этого сообщения в самом сообщении для последующего удаления
      cmd += bot.lastBotMessage();
      menu = fb::InlineMenu(takeAdminStr, cmd.c_str());
      fb::MenuEdit edMenu;
      edMenu.messageID = bot.lastBotMessage();
      edMenu.chatID = u.message().from().id();
      edMenu.setInlineMenu( menu );
      bot.editMenu(edMenu, false);
/*
      res = bot.editMenu(edMenu);
      res.getRaw().printTo(Serial);
      Serial.println();
      Serial.println(bot.lastBotMessage());
*/
      // takeAdmin.msgId=bot.lastBotMessage();
      // takeAdmin.userId=u.message().from().id();
      // уже отослано. не надо посылать
      message.text = ((char*)0);
    }
  }
};
