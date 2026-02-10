#pragma once
#include <StringUtils.h>

//#define toTelegramAsCode(...) TelegramMD::asCode( )
namespace MARKDOWN_TG{
    static constexpr const char chars[] PROGMEM = "_*[]()~`>#+-=|{}.!'\"\\";
    
    static String escape(const char c)
    {
        String dest;
        for ( int i=0; i<22; i++){
            if( c == chars[i]){
                dest += '\\';
                break;
            }
        }
        dest += c;
        return dest;
    };
    static String escape(const char * txt)
    {
        String dest;
        dest.reserve( 2*strlen( txt));
        int j=0;
        while ( txt[j] != '\0'){
            dest += escape( txt[j]);

            j++;
        }
        return dest;
    };
    inline static String escape(const String& str){
        return escape( str.c_str());
    };
};

namespace TelegramMD {
    const char newLine(){ return '\n'; };

    String textIn(const char * txt, const char Q, const char Q2='\0', String (*encode)(const char)=nullptr )
    {
        String out; 
        //String out = encode ? encode(Q) : String(Q);
        out.reserve( 10+strlen(txt));
        out = encode ? encode(Q) : String(Q);
        out += txt;
        if ( encode ) {
            out += Q2 ? encode(Q2) : encode(Q);
        } else { 
            out += Q2 ? Q2 : Q;
        }

        return out;
    };
    inline String textIn(const String& txt, const char Q, const char Q2='\0', String (*encode)(const char)=nullptr  ){ 
        return textIn( txt.c_str(), Q, Q2, encode );
    };

    inline String textIn_(const char * txt, const char Q, const char Q2='\0', String (*encode)(const char)=nullptr ){
        return textIn( txt, Q, Q2, encode ) + ' ';
    }
    inline String textIn_(const String& txt, const char Q, const char Q2='\0', String (*encode)(const char)=nullptr  ){ 
        return textIn_( txt.c_str(), Q, Q2, encode );
    };


    inline String asCode(const String& txt){
        return textIn_(txt, '`'); 
    };
    inline String asCode(const char* txt){
        return textIn_(txt, '`'); 
    };

    //(void (*f)(uint),
    inline String asItallic(const char* txt, String (*encode)(const char *)=nullptr){
        return ( encode == nullptr) ?
            textIn_( txt, '_') : 
            textIn_( encode(txt), '_');
    };    
    inline String asItallic(const String& txt, String (*encode)(const char *)=nullptr ){
        return asItallic( txt.c_str(), encode);
    }

    inline String asBold(const char* txt, String (*encode)(const char *)=nullptr ){
        return ( encode == nullptr) ? 
            textIn_(txt, '*') : 
            textIn_( encode(txt), '*'); 
    };
    inline String asBold(const String& txt, String (*encode)(const char *)=nullptr ){
        return asBold(txt.c_str(), encode); 
    };


    inline String linkTo( const char * txt, const char * link, String (*encode)(const char *)=nullptr ){
        String out;
        out += textIn( ( encode == nullptr) ? txt : encode(txt), '[',']');        
        //out.trim(); // del space
        out += textIn(link, '(',')');
        return out;
    }
};