#include "relay.h"
//#define THIS_BOT_LINK "https://t.me/rtu5024_bot"

#include "core/types/Message.h"

#pragma once
#define NUL_STR ((char*)0)

#include "myFastBotClient.h"
//#include "fsSettings.h"
#include "newFsSettings.h"

/*
struct TakeAdminT {
  long msgId;
  long long userId;
} takeAdmin;
*/
//static constexpr char this_bot_link[] PROGMEM = THIS_BOT_LINK; //"t.me/rtu5024_bot";
// static constexpr char noAdmin_MD[] PROGMEM = "*У меня пока нет хозяина\\. Хочешь им стать\\?*";
// //static constexpr char takeAdminStr[] PROGMEM = "Стать администратором";
// static constexpr char haveAdmin_MD[] PROGMEM = "*У меня уже есть хозяин\\!*\n_Но ты всегда можешь завести [своего бота](" THIS_BOT_LINK ")_";
// static constexpr char haveAdmin_Alert[] PROGMEM = "У меня уже есть хозяин!\nНо ты всегда можешь завести своего бота ";
// static constexpr char youAdminAlready_MD[] PROGMEM = "*Вы уже являетесь администратором\\.*\n_Для помощи отправь_ `/help`";

static constexpr char noAdmin[] PROGMEM = "У меня пока нет хозяина. Хочешь им стать?";
static constexpr char takeAdminStr[] PROGMEM = "Стать администратором";
static constexpr char haveAdmin[] PROGMEM = "У меня уже есть хозяин!\n";
static constexpr char youCanTake[] PROGMEM = "Но ты всегда можешь завести ";
static constexpr char youBot[] PROGMEM = "своего бота";
static constexpr char youAdminAlready[] PROGMEM = "Вы уже являетесь администратором.";
static constexpr char forHelp[] PROGMEM = "\nДля помощи отправь "; //`/help`";
//extern SETTINGS::SettingsT settings;
extern BotSettings::Settings settingsNew;

//обработка команды /start
void handleStart(fb::Update& u, fb::Message& message) {
  // будем беседовать с отправителем
  //message.chatID = u.message().from().id();
  
  fb::MyCommands commands("help;start_portal;start_web;stop_web", "Помощь;Запустить CaptivеPortal;Запустить вебпортал;Остановить вебпортал");
  bot.setMyCommands(commands);
  

  if ( u.message().from().id() == settingsNew.getAdminId() ){ //.admin) {
    String user(u.message().from().username());
    debugPrintf("Admin '%s'[#%s] start command\n", user,
                  (String)(u.message().from().id()));
 
    message.text = TelegramMD::asBold( youAdminAlready, MARKDOWN_TG::escape); //_MD; // F("*Вы уже являетесь администратором\\.*\n_Для помощи отправь_ `/help`");
    String help( TelegramMD::asItallic( forHelp, MARKDOWN_TG::escape ));
    help += /* TelegramMD::asCode */ F( "/help" );
    message.text += help;
    
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
      message.text = TelegramMD::asBold( haveAdmin,  MARKDOWN_TG::escape ); //haveAdmin_MD; //F("*У меня уже есть хозяин\\!*\n_Но ты всегда можешь завести [своего бота](" THIS_BOT_LINK ")_");
      String S(youCanTake );
      S += TelegramMD::textIn( youBot, '[', ']');
      S += TelegramMD::textIn(  App::getHomePage(), '(',')' );
      message.text += TelegramMD::asItallic( S );

    } else {
      String user(u.message().from().username());
      debugPrintf("New user '%s'[#%s] start command\n",
                    user,
                    (String)(u.message().from().id()));
      //debugPrintln("Registri new admin?");

      String cmd = "ta~";
      //cmd += bot.lastBotMessage()+1;
      
      fb::InlineMenu menu(takeAdminStr, cmd.c_str());
      message.text = TelegramMD::asBold( noAdmin, MARKDOWN_TG::escape ); //noAdmin_MD; //F("*У меня пока нет хозяина\\. Хочешь им стать\\?*");
      message.setInlineMenu(menu);
      auto res = bot.sendMessage(message);
      if ( res.valid() ){
      // и запоминаем id этого сообщения в самом сообщении для последующего удаления
        cmd += bot.lastBotMessage();
        menu = fb::InlineMenu(takeAdminStr, cmd.c_str());
        fb::MenuEdit edMenu;
        edMenu.messageID = bot.lastBotMessage();
        edMenu.chatID = u.message().from().id();
        edMenu.setInlineMenu( menu );
        bot.editMenu(edMenu, false);
      }
      message.text = ((char*)0);
    }
  }
};
