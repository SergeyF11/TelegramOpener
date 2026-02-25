#pragma once

#include <FastBot2.h>

namespace Report {
    enum To {
        None,
        Admin,
        Channel,
    };

    const char * toStr(const To r ){
        switch(r){
            case Admin: return "Admin";
            case Channel : return "Channel";

            default: return "None";
        }
    };

    To to = None;
    const char * toStr(){
        return toStr(to );
    };

    void setReportTo(const To r){
        to = r;
    };

    void setReportTo( const String& s){
        if ( s.equalsIgnoreCase( toStr( Admin ))){
            to = Admin;
        } else if ( s.equalsIgnoreCase( toStr( Channel ))){
            to = Channel;
        } else 
            to = None;
    };
    // int64_t reportTo(){

    // }
    bool needRecreateKeyboard = false;

}

#include "newFsSettings.h"

extern BotSettings::Settings settings;

namespace ReportSheduler {
    struct Opener {
        int64_t id;
        String userName;
        String firstName;
        String lastName;
        Opener* next = nullptr;
        Opener() : id(0) {
            userName.reserve(64);
            firstName.reserve(64);
            lastName.reserve(64);
            clean();
        }
        Opener( fb::UserRead& opener)  : Opener()
        {
            *this = opener; 
            //next = new Opener;
        };
        ~Opener(){
            clean();
        }
        Opener(const Opener&) = delete;
        Opener& operator=(const Opener&) = delete;

        // add to end
        Opener& operator=( fb::UserRead& opener){
            Opener * current = this;
            if ( has() ){
                // Ищем конец
                while( current->hasNext() ){
                    current = current->next;
                }
                current->next = new Opener;
                current = current->next;
            } /* else
                active = this;
             */
            current->id = opener.id().toInt64();
            current->userName = opener.username().toString();
            current->firstName = opener.firstName().toString();
            current->lastName = opener.lastName().toString();
            
            return (*this);
        };
        
        void clean(){
            Serial.println("clean");Serial.flush();

            if( ! has() ) return;
            if( hasNext()) {

                Serial.println("clean next"); Serial.flush();
                next->clean();

                Serial.println("delete next");Serial.flush();
                delete next;
                next = nullptr;
                return;
            }
            id = 0LL;

            Serial.println("Sheduler cleaned");Serial.flush();
        }

        bool has() const { return id != 0; }
        bool hasNext() const {
            return next != nullptr; // && next->has();
        }

    };
    static Opener opener;
}

bool sendReport( const ReportSheduler::Opener& opener /* = ReportSheduler::opener  */);

bool sheduleReport( fb::UserRead& _opener ) {
    ReportSheduler::opener = _opener;
    return ReportSheduler::opener.has();
}

bool sendReport(  fb::UserRead& _opener ) {
    //ReportSheduler::Opener opener(_opener);
    ReportSheduler::opener = _opener;
    if( ReportSheduler::opener.has())
        return sendReport(ReportSheduler::opener);
    else    
        return false;
}


bool sendReport( const ReportSheduler::Opener& opener /* = ReportSheduler::opener  */){
    
    static constexpr  char TMPL_OPEN[] PROGMEM = "<i>Открыл</i> <a href=\"tg://user?id=%lld\"> <b>";
    static const char SPOILER[] PROGMEM = "<%stg-spoiler>";
    static constexpr  char START[] = "";
    static constexpr  char END[] = "/";

    //const auto openerId = opener.id().toInt64();
    if ( Report::to == Report::None /* ||
        ( Report::to == Report::Admin && opener.id == settings.getAdminId() ) */ ) return true;
    int64_t reportTo = ( Report::to == Report::Admin ) 
        ? settings.getAdminId() 
        : settings.getChatId();

    if ( reportTo == 0LL ) return true;
    if ( reportTo != settings.getAdminId() ) Report::needRecreateKeyboard = true;

// Буфер с запасом
    char report[256];
    char* ptr = report;
    int remaining = sizeof(report);
    
    // Статическая часть
    int len = snprintf(ptr, remaining, TMPL_OPEN, 
                       opener.id);
    ptr += len;
    remaining -= len;
    
    // Username или ID
    //String usernameStr = opener.username().toString();
    //if (!usernameStr.isEmpty()) {
    if (!opener.userName.isEmpty()) {
        len = snprintf(ptr, remaining, "@%s", opener.userName.c_str());
    } else {
        len = snprintf(ptr, remaining, "#%lld", opener.id);
    }
    ptr += len;
    remaining -= len;
    
    // Закрываем тег
    len = snprintf(ptr, remaining, "</b></a> ");
    ptr += len;
    remaining -= len;
    
    // Имя и фамилия (если есть)
    // String firstNameStr = opener.firstName().toString();
    // String lastNameStr = opener.lastName().toString();
    
    if (!opener.firstName.isEmpty() || !opener.lastName.isEmpty()) {
        len = snprintf(ptr, remaining, SPOILER, START );//"<tg-spoiler>");
        ptr += len;
        remaining -= len;
        
        if (!opener.firstName.isEmpty()) {
            len = snprintf(ptr, remaining, "%s", opener.firstName.c_str());
            ptr += len;
            remaining -= len;
            
            if (!opener.lastName.isEmpty()) {
                len = snprintf(ptr, remaining, " ");
                ptr += len;
                remaining -= len;
            }
        }
        
        if (!opener.lastName.isEmpty()) {
            len = snprintf(ptr, remaining, "%s", opener.lastName.c_str());
            ptr += len;
            remaining -= len;
        }
        
        len = snprintf(ptr, remaining, SPOILER, END ); //"</tg-spoiler>");
        ptr += len;
        remaining -= len;
    }
    
    debugPrintln(report);
    

    // Отправляем
    fb::Message msg(report, reportTo );
    //auto res = bot.sendMessage(adminId, report, "HTML");
    msg.mode = fb::Message::Mode::HTML;
   
    //return bot.sendMessage(msg).type() == fb::Result::Type::OK;
    // auto res = bot.sendMessage(msg, false);
    // return res.valid();

    // opener.clean();
    // return res;
    auto res = bot.sendMessage(msg, true);

    return res.valid() && ! res.isError() ;

}

/* bool sendReport(int64_t adminId, fb::UserRead opener) {
    static constexpr  char TMPL_OPEN[] PROGMEM = "<i>Открыл</i> <a href=\"tg://user?id=%lld\"> <b>";
    static const char SPOILER[] PROGMEM = "<%stg-spoiler>";
    static constexpr  char START[] = "";
    static constexpr  char END[] = "/";

    const auto openerId = opener.id().toInt64();
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
 */