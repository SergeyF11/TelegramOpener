#pragma once
#include <LittleFS.h>

#include "debug.h"

namespace channelName {

  static char * channelNameP = nullptr;
  const char * creat(size_t);
  void clean();
  String fileName(const long long);
  bool check(const long long);
  bool save(const long long );
  bool save(const long long , const char *);
  bool save(const long long , String& );
  const char* get();
  bool isEmpty();

  ///////////////////////////////////////////////////////////////////////

  const char * creat(size_t size){
    if ( channelNameP == nullptr ) 
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
    if ( f.size() != strlen(channelNameP) ) return res;
    char * buf = (char *) malloc(f.size() );
    f.readBytes(buf, f.size());
    res = ( strncmp( buf, channelNameP, f.size()) == 0 ) ;
    free(buf);
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
      f.print(channelNameP);
      delay(1);
      f.close();
    }
    return writed;
  };
  bool save(const long long id, const char *name){
    if ( creat(strlen(name)) != nullptr )
        strcpy(channelNameP, name);
    return save(id);
  };
  //String 
  bool save(const long long id, String& name){
  if ( creat(name.length()) != nullptr )
      strcpy(channelNameP, name.c_str());
  return save(id);
  };

  const char* get(){
//    debugPretty; debugPrintln( channelNameP);
    return channelNameP;
  };

  //String 
  const char *load(const long long id){
//    debugPretty;
//  String name((char *)(NULL));
//  if ( channelName.isEmpty() ) 
    if ( LittleFS.exists(fileName(id).c_str())) {
        auto f = LittleFS.open( fileName(id).c_str(), "r");
        if( creat(f.size()) != nullptr ){
            // String s = f.readString();
            // strcpy(channelName, s.c_str());
            f.readBytes(channelNameP, f.size());
            channelNameP[f.size()] = '\0';
        }
        f.close();
    }    
//    debugPrintln( channelNameP);  
    return channelNameP;
  };

  bool isEmpty(){
    return channelNameP == nullptr;
  };
}
