#pragma once
#include <LittleFS.h>
#include <StringUtils.h>
#include "TelegramMD.h"

#include "debug.h"

namespace channelName {

  char * channelNameP = nullptr;
  const char * creat(size_t);
  void freeMemory();
  String fileName(const long long);
  bool check(const long long);
  bool save(const long long );
  bool save(const long long , const char *);
  bool save(const long long , String& );
  const char* get();
  bool isEmpty();

  ///////////////////////////////////////////////////////////////////////

  const char * creat(size_t size){
    freeMemory();
    channelNameP = (char *)malloc(size);
    return channelNameP;
  };
  void freeMemory(){
     if ( channelNameP != nullptr ){
        free(channelNameP);
        channelNameP = nullptr;
     }
  };
  String fileName(const long long id){
    String fileName(id);
    fileName += F(".nm");
    return fileName;
  };
  // check file content equals channelNameP

  bool check(const long long id){
    bool res = false;
    auto fn = fileName(id);
    if ( ! LittleFS.exists(fn.c_str())) return res;
    auto f = LittleFS.open( fn.c_str(), "r");
    if ( ! f ) return res;
    if ( f.size() != strlen(channelNameP) ){
      char * buf = (char *) malloc(f.size() );
      f.readBytes(buf, f.size());
      res = ( strncmp( buf, channelNameP, f.size()) == 0 ) ;
      free(buf); 
    } 
    f.close();
    return res;
  };

  bool save(const long long id) {  
    //check
    {
      if ( check(id) ) return true;
    }
    debugPretty;
    debugPrintln(channelNameP);
    
    auto f = LittleFS.open( fileName(id).c_str(), "w");
    int writed=0;
    if ( f ) {
      writed=f.print(channelNameP);
      delay(1);
      f.close();
    }
    return writed;
  };
  bool save(const long long id, const char *name){
    freeMemory();
    if ( creat(strlen(name)) != nullptr ){
      strcpy(channelNameP, name);
      return save(id);
    }
    return false;
  };
  //String 
  bool save(const long long id, String& name){
  if ( creat(name.length()+1) != nullptr )
      strcpy(channelNameP, name.c_str());
  return save(id);
  };

  const char* get(){
    return channelNameP;
  };

  //String 
  const char * load(const long long id){
    freeMemory();
    String fn = fileName(id);
    if ( LittleFS.exists( fn.c_str() )) {
        auto f = LittleFS.open( fn.c_str(), "r");
        if( creat(f.size()) != nullptr ){
            f.readBytes(channelNameP, f.size());
            channelNameP[f.size()] = '\0';
        }
        f.close();
    }    

    return channelNameP;
  };

  bool isEmpty(){
    return channelNameP == nullptr || channelNameP[0] == '\0';
  };
  String addChannelName( String& text, long long chatId){
    //String text;
    load(chatId);
    if ( chatId != 0ll ) {
      //message.text += F("\nМой канал управления *");
      text += '\n';
      text += CHANNEL_FOR_CONTROL;
      text += F("*");
      if ( isEmpty() ){
        text += "\\#\\"; 
//                                                    1001715239030ll
        text += 1000000000000ll + chatId;
      } else { 
        text += F("\\'");
        text += get(); 
        text += F("\\'");
      }
      text += F("*");
    }
    return text;
  };

  String addChannelName( long long chatId, char prefix = '\0'){
    String text( prefix );
    if ( chatId != 0ll ) {
      if ( isEmpty() ) load(chatId);
      //message.text += F("\nМой канал управления *");
      //text += '\n';
      // if( prefix != '\0' ) 
      //   text += prefix;
      text += CHANNEL_FOR_CONTROL;
      if ( isEmpty() ){
        text += TelegramMD::asBold( String('#') + (1000000000000ll + chatId) );
      } else {
        text += TelegramMD::asBold( TelegramMD::textIn( get(), '\'' ));
      }
//       text += F("*");
//       if ( isEmpty() ){
//         text += "\\#\\"; 
// //                                                    1001715239030ll
//         text += 1000000000000ll + chatId;
//       } else { 
//         text += F("\\'");
//         text += get(); 
//         text += F("\\'");
//      }
//      text += F("*");
    }
    return text;
  };
}
