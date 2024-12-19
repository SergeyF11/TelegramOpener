#pragma once
#include <LittleFS.h>

#include "debug.h"

class LastMsg {
  public:
  LastMsg(){
    if(_suffix != nullptr){
      free(_suffix);
    }
  };
  LastMsg(const long long chatId, const uint id=0, const char* suffix=nullptr) /* : _suffix(suffix) */ {
    debugPretty;
    this->init(chatId, id);
    // if ( suffix == nullptr) debugPrintln("nullptr");
    // else debugPrintln(suffix);
    if( suffix != nullptr ){
      //_suffix = suffix;
      _suffix = (char *)malloc( strlen(suffix));
      strcpy( _suffix, suffix);
      debugPrintln(_suffix );
    }
  };
  ~LastMsg(){};
  
  void init(const long long chatId, const uint id=0){
    this->chatId = chatId;
    if ( id == 0 ) this->id = this->_get();
    else this->id = id;
  };
  uint32_t get(){
    return this->id;
  }
  // uint get(bool fromFile = false ){
  //   //Serial.println( __PRETTY_FUNCTION__); 
  //   if ( ! fromFile && this->id ) return this->id;
  //   return this->_get();
  // };
  bool clean(){
    debugPretty;
    debugPrintf("Remove %s\n", this->name().c_str());
    return LittleFS.remove(this->name().c_str());
  };
  bool set(uint32_t id =0 ){
    //fileName += suffix;
    File f = LittleFS.open(this->name().c_str(),"w");
    if ( ! f ) return f;
    
    uint32_t writeId = id ? id : this->id;
    this->id = writeId;
    int writedBytes = f.print(writeId);
    delay(1);
    f.close();
    return writedBytes;
  };
  private:
  String name(){
    debugPretty;
    String fileName;
    fileName += this->chatId;
    fileName += _suffix;
    // if ( this->_suffix != nullptr ){
    //   fileName += this->_suffix;
    //   debugPrint("add suffix "); debugPrintln( this->_suffix);
    // }
    debugPrintln(fileName);
    return fileName;
  };
  uint32_t _get(){
    uint id=0;
    // Serial.println( __PRETTY_FUNCTION__); 
    File f = LittleFS.open(this->name().c_str(),"r");
    if ( f ){
      String read = f.readString();
      id = read.toInt();
      f.close();
    }
    //debugPrintln(id);
    return id;
  };

  long long chatId;
  uint32_t id;
  char* _suffix=nullptr;
};
