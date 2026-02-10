#pragma once

#include <FastBot2.h>


bool sendReport( long adminId, fb::UserRead opener ){
    auto openerId = opener.id().toInt64();
    if ( openerId == settings.getAdminId() ) return true;

    String reportStr; reportStr.reserve(200);
    reportStr += F("<i>Открыл</i> <a href=\"tg://user?id=");
    opener.id().addString( reportStr );
    reportStr += F("\"> <b>");
    if ( ! opener.username().toString().isEmpty() ) {
        reportStr +='@';
        opener.username().addString( reportStr );
    } else {
        reportStr += '#';
        opener.id().addString( reportStr );
    }
    reportStr += F("</b></a> ");

    String from_name( opener.firstName().toString() );

    if(  !from_name.isEmpty() ) from_name += ' ';
    opener.lastName().addString( from_name );
    
    if ( !from_name.isEmpty() ){
        reportStr += F("<tg-spoiler>");
        reportStr += from_name;
        reportStr += F("</tg-spoiler>");
    }
    // txt += ;
    // txt += " открыл";
    fb::Message msg; //("Будем открывать", My_Telegram_id );
    msg.text = reportStr;
    msg.chatID = adminId;
    
    Serial.println( reportStr );

    
    msg.mode = fb::Message::Mode::HTML;
    return bot.sendMessage(msg).type() == fb::Result::Type::OK;
}