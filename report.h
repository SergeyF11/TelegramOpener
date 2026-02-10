#pragma once

#include <FastBot2.h>

bool sendReport(int64_t adminId, fb::UserRead opener) {
    static constexpr const char TMPL_OPEN[] PROGMEM = "<i>Открыл</i> <a href=\"tg://user?id=%lld\"> <b>";
    static const char SPOILER[] PROGMEM = "<%stg-spoiler>";
    static constexpr const char START[] = "";
    static constexpr const char END[] = "/";

    auto openerId = opener.id().toInt64();
    if (openerId == adminId) return true;

    // Буфер с запасом
    char report[256];
    char* ptr = report;
    int remaining = sizeof(report);
    
    // Статическая часть
    int len = snprintf(ptr, remaining, TMPL_OPEN, 
                       openerId);
    ptr += len;
    remaining -= len;
    
    // Username или ID
    String usernameStr = opener.username().toString();
    if (!usernameStr.isEmpty()) {
        len = snprintf(ptr, remaining, "@%s", usernameStr.c_str());
    } else {
        len = snprintf(ptr, remaining, "#%lld", openerId);
    }
    ptr += len;
    remaining -= len;
    
    // Закрываем тег
    len = snprintf(ptr, remaining, "</b></a> ");
    ptr += len;
    remaining -= len;
    
    // Имя и фамилия (если есть)
    String firstNameStr = opener.firstName().toString();
    String lastNameStr = opener.lastName().toString();
    
    if (!firstNameStr.isEmpty() || !lastNameStr.isEmpty()) {
        len = snprintf(ptr, remaining, SPOILER, START );//"<tg-spoiler>");
        ptr += len;
        remaining -= len;
        
        if (!firstNameStr.isEmpty()) {
            len = snprintf(ptr, remaining, "%s", firstNameStr.c_str());
            ptr += len;
            remaining -= len;
            
            if (!lastNameStr.isEmpty()) {
                len = snprintf(ptr, remaining, " ");
                ptr += len;
                remaining -= len;
            }
        }
        
        if (!lastNameStr.isEmpty()) {
            len = snprintf(ptr, remaining, "%s", lastNameStr.c_str());
            ptr += len;
            remaining -= len;
        }
        
        len = snprintf(ptr, remaining, SPOILER, END ); //"</tg-spoiler>");
        ptr += len;
        remaining -= len;
    }
    
    debugPrintln(report);
    
    // Отправляем
    fb::Message msg(report, adminId );
    //auto res = bot.sendMessage(adminId, report, "HTML");
    msg.mode = fb::Message::Mode::HTML;
   
    //return bot.sendMessage(msg).type() == fb::Result::Type::OK;
    auto res = bot.sendMessage(msg, true);
    return res.valid() && ! res.isError() ;
}

// bool sendReport( long adminId, fb::UserRead opener ){
//     auto openerId = opener.id().toInt64();
//     if ( openerId == settings.getAdminId() ) return true;

//     String reportStr; reportStr.reserve(200);
//     reportStr += F("<i>Открыл</i> <a href=\"tg://user?id=");
//     opener.id().addString( reportStr );
//     reportStr += F("\"> <b>");
//     if ( ! opener.username().toString().isEmpty() ) {
//         reportStr +='@';
//         opener.username().addString( reportStr );
//     } else {
//         reportStr += '#';
//         opener.id().addString( reportStr );
//     }
//     reportStr += F("</b></a> ");

//     String from_name( opener.firstName().toString() );

//     if(  !from_name.isEmpty() ) from_name += ' ';
//     opener.lastName().addString( from_name );
    
//     if ( !from_name.isEmpty() ){
//         reportStr += F("<tg-spoiler>");
//         reportStr += from_name;
//         reportStr += F("</tg-spoiler>");
//     }
//     // txt += ;
//     // txt += " открыл";
//     fb::Message msg; //("Будем открывать", My_Telegram_id );
//     msg.text = reportStr;
//     msg.chatID = adminId;
    
//     Serial.println( reportStr );

    
//     msg.mode = fb::Message::Mode::HTML;
//     return bot.sendMessage(msg).type() == fb::Result::Type::OK;
// }