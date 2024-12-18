#pragma once
#include <StringUtils.h>

//#define toTelegramAsCode(...) TelegramMD::asCode( )
namespace MARKDOWN_TG{
    static const char chars[] PROGMEM = "_*[]()~`>#+-=|{}.!\'\"";
    static String escape(const char * txt){
        String dest;
        dest.reserve( 2*strlen( txt));
        int j=0;
        while ( txt[j] != '\0'){
            int i=0;
            while ( chars[i] != '\0' ){
                if ( txt[j] == chars[i] ) {
                    dest += '\\';
                    break;
                }
                i++;
            }
            dest += txt[j];
            j++;
        }
        return dest;
    };
    static String escape(const String& str){
        return escape( str.c_str());
    };
}
namespace TelegramMD {

    // String textIn(String& txt, const char Q ){ 
    //     String out(Q);
    //     out.reserve(2+txt.length());
    //     out += txt;
    //     out += Q;
    //     return out;
    //     return textIn(txt.c_str(), Q);
    // };
    String textIn(const char * txt, const char Q ){
        size_t txtLen = strlen(txt);
        char * out = (char*)malloc(3+ txtLen);
        out[0] = Q;
        out[1+txtLen] = Q;
        strncpy(out+1, txt, txtLen);
        out[2+txtLen] = '\0';
        String s(out);
        free(out);
        return s;
    };    
    String textIn(const String& txt, const char Q ){ 
        return textIn(txt.c_str(), Q);
    };
    String asCode(const String& txt){
        return textIn(txt, '`'); 
    };
    String asItallic(const String& txt){
        return textIn( MARKDOWN_TG::escape(txt.c_str()), '_'); 
    };
    String asItallic(const char* txt){
        return textIn( MARKDOWN_TG::escape(txt), '_'); 
    };
    String asBold(const String& txt){
        return textIn(MARKDOWN_TG::escape(txt.c_str()), '*'); 
    };
    String asBold(const char* txt){
        return textIn(MARKDOWN_TG::escape(txt), '*'); 
    };
}