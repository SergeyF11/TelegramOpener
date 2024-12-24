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

    String textIn(const char * txt, const char Q, const char Q2='\0' ){
        //String _txt(txt);
        String out(Q);
        out.reserve( 3+strlen(txt));
        out += txt;
        out += Q2 ? Q2 : Q;
        return out;
    };
    String textIn(const String& txt, const char Q, const char Q2='\0'  ){ 
        return textIn( txt.c_str(), Q, Q2);
    };


    // String textIn(const char * txt, const char Q, const char Q2='\0' ){
    //     size_t txtLen = strlen(txt);
    //     char * out = (char*)malloc(3+ txtLen);
    //     out[0] = Q;
    //     out[1+txtLen] = Q2 ? Q2 : Q;
    //     strncpy(out+1, txt, txtLen);
    //     out[2+txtLen] = '\0';
    //     String s(out);
    //     free(out);
    //     return s;
    // };    
    // String textIn(const String& txt, const char Q, const char Q2='\0' ){ 
    //     return textIn(txt.c_str(), Q, Q2);
    // };


    String asCode(const String& txt){
        return textIn(txt, '`'); 
    };
    String asCode(const char* txt){
        return textIn(txt, '`'); 
    };

    //(void (*f)(uint),
    String asItallic(const char* txt, String (*encode)(const char *)=nullptr){
        return ( encode == nullptr) ?
            textIn( txt, '_') : 
            textIn( encode(txt), '_');
    };    
    String asItallic(const String& txt, String (*encode)(const char *)=nullptr ){
        return asItallic( txt.c_str(), encode);
    }
    //     if ( encode == nullptr)
    //         return textIn( txt.c_str(), '_'); 
    //     return textIn( encode(txt.c_str()), '_'); 
    // };
    // String asItallic(const String& txt ){
    //     return textIn( txt.c_str(), '_'); 
    // };
    String asBold(const char* txt, String (*encode)(const char *)=nullptr ){
        return ( encode == nullptr) ? 
            textIn(txt, '*') : 
            textIn( encode(txt), '*'); 
    };
    String asBold(const String& txt, String (*encode)(const char *)=nullptr ){
        return asBold(txt.c_str(), encode); 
    };


    // String asItallicMD(const String& txt){
    //     return textIn( MARKDOWN_TG::escape(txt.c_str()), '_'); 
    // };
    // String asItallicMD(const char* txt){
    //     return textIn( MARKDOWN_TG::escape(txt), '_'); 
    // };
    // String asBoldMD(const String& txt){
    //     return textIn(MARKDOWN_TG::escape(txt.c_str()), '*'); 
    // };
    // String asBoldMD(const char* txt){
    //     return textIn(MARKDOWN_TG::escape(txt), '*'); 
    // };

    static const char _http[] PROGMEM = "http://";

    String linkTo( const char * txt, const char * link, String (*encode)(const char *)=nullptr ){
        String out;
        out += textIn( ( encode == nullptr) ? txt : encode(txt), '[',']');
        
        String _link;
        _link.reserve( 10 + strlen(link));
        if ( strncmp(link, _http, 4) )
            _link += link;
        else {
            _link += _http;
            _link += link;
        }
        out += textIn(link, '(',')');
        return out;
    }
}