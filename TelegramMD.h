#pragma once
#include <StringUtils.h>

//#define toTelegramAsCode(...) TelegramMD::asCode( )
namespace MARKDOWN_TG{
    static const char chars[] PROGMEM = "_*[]()~`>#+-=|{}.!'\"\\";
    
    static String escape(const char c){
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
    static String escape(const char * txt){
        String dest;
        dest.reserve( 2*strlen( txt));
        int j=0;
        while ( txt[j] != '\0'){
            dest += escape( txt[j]);
        //     int i=0;
        //     while ( chars[i] != '\0' ){
        //         if ( txt[j] == chars[i] ) {
        //             dest += '\\';
        //             break;
        //         }
        //         i++;
        //     }
        //     dest += txt[j];
            j++;
        }
        return dest;
    };
    static String escape(const String& str){
        return escape( str.c_str());
    };
}
namespace TelegramMD {
    const char newLine(){ return '\n'; };
    String textIn(const char * txt, const char Q, const char Q2='\0', String (*encode)(const char)=nullptr ){
        //String _txt(txt);
        String out = encode ? encode(Q) : String(Q);
        out.reserve( 3+strlen(txt));
        out += txt;
        if ( encode ) {
            out += Q2 ? encode(Q2) : encode(Q);
        } else { 
            out += Q2 ? Q2 : Q;
        }
        out += ' ';
        return out;
    };
    String textIn(const String& txt, const char Q, const char Q2='\0', String (*encode)(const char)=nullptr  ){ 
        return textIn( txt.c_str(), Q, Q2, encode );
    };


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

    String asBold(const char* txt, String (*encode)(const char *)=nullptr ){
        return ( encode == nullptr) ? 
            textIn(txt, '*') : 
            textIn( encode(txt), '*'); 
    };
    String asBold(const String& txt, String (*encode)(const char *)=nullptr ){
        return asBold(txt.c_str(), encode); 
    };


    String linkTo( const char * txt, const char * link, String (*encode)(const char *)=nullptr ){
        String out;
        out += textIn( ( encode == nullptr) ? txt : encode(txt), '[',']');        
        out.trim(); // del space
        out += textIn(link, '(',')');
        return out;
    }
}