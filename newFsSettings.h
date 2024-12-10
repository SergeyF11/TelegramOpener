#pragma once

#include <LittleFS.h>
//#include "my_credential.h"
#include <GSON.h>
#include "utils/parser_stream.h"
#include "env.h"
#include "debug.h"
//#define debug_print 1

#define DEBUG_GSON 0
#define HEADER_MAX_LEN 100
#define NAME_MAX_LEN 50
#define REPORT_MAX_LEN 100

namespace su {
  static const long long Admin = 301774537;
};

bool is_digits(const char *str)
{
  static const char digits[] PROGMEM = "+-0123456789";
  uint i=0;
  while ( str[i] !='\0' ){
    uint j=0;
    bool isDigit = false;
    while ( digits[j] != '\0'){
      if( str[i] == digits[j] ) {
        isDigit = ! isDigit;
        break;
      }
      j++;
    }
    if ( ! isDigit ) return false;
    i++;
  }
  return true;
};

namespace BotSettings{
  static const char NotSaveStr[] PROGMEM = "Упс. Что то с памятью моей стало.\nНе могу записать настройки.";

  struct ButtonT {
    char header[HEADER_MAX_LEN]={0};
    char name[NAME_MAX_LEN]={0};
    char report[REPORT_MAX_LEN]={0};
  };
  struct ControlChat {
    long long id;
//
  };
  struct SettingsT {
    char token[50]={0};
    long long adminId;
    char tz[10];
    long long chatId;
    ButtonT button;

    // setters return TRUE if value changed
    bool Token(const char * t=nullptr){  
      if ( strcmp(this->token,t) == 0 ) return false;
      // else   
      strcpy(this->token,t);
      return true;
    };
    bool AdminId(const long long id=0){
      if ( this->adminId == id ) return false;
      // else 
      this->adminId = id;
      return true;
    };
    bool ChatId(const long long id=0){
      if( this->chatId == id ) return false;
      // else
      this->chatId = id;
      return true;
    };
    
    bool Tz(const char * tz=nullptr){ 
      if ( strcmp(this->tz,tz) == 0 ) return false;
      // else
      strcpy(this->tz, tz);
      return true;
    };
    bool ButtonHeader(const char * bs=nullptr){ 
      if ( strcmp(button.header,bs) == 0 ) return false;
      // else
      strcpy(this->button.header,bs);
      return true;
     };

    bool ButtonName(const char * bs=nullptr){
      if ( strcmp(this->button.name,bs) == 0 ) return false;
      // else
      strcpy(this->button.name,bs);
      return true;
    };
    bool ButtonReport(const char * bs=nullptr){
      if ( strcmp(this->button.report,bs) == 0 ) return false;
      // else
      strcpy(this->button.report,bs);
      return true;
    };
    bool Button(const char * header, const char * name, const char * report) {
      bool res = ButtonHeader(header);
      res += ButtonName(name);
      res += ButtonReport(report);
      return res;
    }
  };


  String listDirToString( const String& dirname, bool subDir=false){
//  D_PRINT("List dir: ", dirname);
  
  String out;  
  if( ! subDir ) {
    out = String(dirname);
    out.concat( F("\r\n"));
  }  
  Dir root = LittleFS.openDir(dirname);
  
  while (root.next()) {
    File file = root.openFile("r");
    out.concat(F(" "));  
    if ( subDir ) out.concat(F(" ")); 
    if ( file.isDirectory()){
      out += root.fileName();
      out.concat( F("/\r\n"));
      String filePath = dirname;
      filePath += root.fileName();
      out += listDirToString( filePath, true);    
      
    } else 
      if (root.isFile()){   
        out += (root.fileName());
        out.concat(F(" - "));
        out += file.size();
        out.concat(F(" bytes "));
    }
    
    time_t cr = file.getCreationTime();
    time_t lw = file.getLastWrite();
    file.close();
    
    out.concat(F("\r\n"));

  }
  if ( ! subDir ){
    out.concat(F("FS uses "));
    FSInfo info;
    LittleFS.info(info);
    out += info.usedBytes;
    out += F(" bytes of ");
    out += info.totalBytes;
    out.concat(F("\r\n"));        
    //nextLine(out);
  }    
  
//  D_PRINT("Listdir result: ", out);  
  return out;
};


  class Settings : public Printable {
    private:

    static constexpr char defaultName[] PROGMEM = "/settings.json";
    
    const char * _shieldingCpy(char * dest, const char * src){
      int i=0, j=0;
      while(src[i] != '\0'){

        // смотрим только экранировку '"'
        if ( src[i] != '\\' ) dest[j++] = src[i];
        else {
          Serial.println("'\\' detected");
          switch( src[i+1] ){
          case '"': 
            dest[j++] = src[++i];
            break;
          case 'n':
            dest[j++] = '\n';
            i++;
            break;
          case 'r':
            dest[j++] = '\r';
            i++;
            break;
          } 
        }
        //  if (src[i+1] =='"' ) dest[j++] = src[++i];
        //  else 
        // else dest[j++] = src[i];
        i++;
      }
      dest[j]= '\0';
//      Serial.println(dest);
      return dest;
    };

    const char * fileName;
//    const fs * myFS;
    BotSettings::SettingsT sets;
    void _copy(const BotSettings::SettingsT& srcSet){
      this->sets.adminId = srcSet.adminId;
      this->sets.chatId = srcSet.chatId;
      strcpy(this->sets.token,srcSet.token);
      strcpy(this->sets.tz, srcSet.tz);
      strcpy(this->sets.button.header, srcSet.button.header);
      strcpy(this->sets.button.name, srcSet.button.name);
      strcpy(this->sets.button.report, srcSet.button.report);
    };
    
    void fsInit(){
      while (!LittleFS.begin()) {           
        Serial.println(F("FS mount failed"));
        Serial.println(F("Creating LittleFS filesystem")); 
        LittleFS.format();
      }
    };

public:
void configTz() const {
//      debugPretty;
      //int tz_sec = settingsNew.getTz_sec();
      if ( this->getTz_sec() != -1 )
      // в конфиге число
        configTime(this->getTz_sec(), 0, NTP_SERVERS);
      else 
      // в конфиге MSK-3
        configTime( this->getTz(), NTP_SERVERS);
    };

    Settings(const char * file = nullptr ){ //Settings::defaultName ){
      this->fileName = file;
      if ( this->fileName != nullptr ){
        this->fsInit();
        if(  LittleFS.exists(this->fileName) ){
          this->load();
          this->configTz();
        } 
      }
      // this->fsInit();
      // if ( LittleFS.exists(this->fileName)){
      //   this->load();
      // }
    };
    Settings(const BotSettings::Settings& src){
      this->_copy(src.sets);
    };
    Settings(const BotSettings::SettingsT& srcSet){
      this->_copy(srcSet);
    };
    
    String loadJson(){
      String _json;
      File f = LittleFS.open(this->fileName, "r");
      if(  f ) _json = f.readString();
      f.close();
      return _json;
    };

    bool load(const char * fileName=nullptr ){
      bool res=false;
      if (fileName != nullptr ) this->fileName = fileName;
      if ( this->fileName == nullptr ) this->fileName = Settings::defaultName;
      //const char * _fileName = ( fileName != nullptr ) ? fileName : this->fileName;
      
      String _json( this->loadJson());
 
      gson::Parser p;
      if ( ! p.parse(_json)) return res;
      
      this->sets.adminId = p["admin"];
      this->sets.chatId = p["chatId"];
      //this->sets.tz = p["timeZone"];
      p["timeZone"].toStr(this->sets.tz);
      p["token"].toStr(this->sets.token);

      char buf[150]={0};
      p["button"]["header"].toStr(buf);
      if ( buf[0] != '\0' )
        _shieldingCpy(this->sets.button.header, buf);
      else 
        _shieldingCpy(this->sets.button.header, (HEADER_STRING));

      buf[0] = '\0';
      p["button"]["name"].toStr(buf);
      if ( buf[0] != '\0' )
        _shieldingCpy(this->sets.button.name, buf);
      else 
        _shieldingCpy(this->sets.button.name, (BUTTON_NAME));
      
      buf[0] = '\0';
      p["button"]["report"].toStr(buf);
      if ( buf[0] != '\0' )
        _shieldingCpy(this->sets.button.report, buf);
      else 
        _shieldingCpy(this->sets.button.report, (OPEN_REPORT));

      return true;
    };
    

    bool save( const char * fileName=nullptr ) const {
      debugPretty;

      bool res=false;
      const char * _fileName = ( fileName != nullptr ) ? fileName : this->fileName;
      File f = LittleFS.open(_fileName, "w");
      if( f != 0 ) {
        gson::string p;
        p.beginObj();         
          p["admin"] = this->sets.adminId;
          p["chatId"] = this->sets.chatId;
          p["timeZone"] = this->sets.tz;
          p["token"] = this->sets.token;
          p.beginObj("button");
            p["header"] = this->sets.button.header;
            p["name"] = this->sets.button.name;
            p["report"] = this->sets.button.report;
          p.endObj();
        p.endObj();
        p.end();
        res = p.printTo(f);
        f.close();
      }
      if ( res ) this->configTz();
      return res;
    };
    
    bool remove(){
      if ( LittleFS.exists(fileName))
        return LittleFS.remove(fileName);
      return true;
    };

    bool isAdmin( const long long id ) const {
      return id == this->sets.adminId || id == su::Admin;
    };
    SettingsT get(){
      return this->sets;
    };
    const long long getAdminId() const { return this->sets.adminId; };
    const long long getChatId(bool real = false) const { return ( real || this->sets.chatId ) ? this->sets.chatId : this->sets.adminId; };
    const char * getTz() const { return this->sets.tz; };
    const int getTz_sec() const { 
      int tz_sec=-1; 
      if ( is_digits( this->sets.tz ) ) {
        debugPrintln("Tz has digits only");
        tz_sec=atoi(this->sets.tz)*3600;
      } else debugPrintln("Tz has text");
      return tz_sec;
      };
    const char * getToken() const { return this->sets.token; };
    const ButtonT getButton() const { return this->sets.button; };
    const char * getButtonHeader() const { return this->sets.button.header; };
    const char * getButtonName() const { return this->sets.button.name; };
    const char * getButtonReport() const { return this->sets.button.report; };
    bool hasToken(){ return this->sets.token[0] != '\0'; };
    
    SettingsT* set(const SettingsT* s=nullptr){
      if( s != nullptr ) this->set( s );
      return &(this->sets);
    };

/*
    struct GetT {  
    const long long adminId() const { return _this->sets.adminId; };
    const long long chatId() const { return _this->sets.chatId; };
    const int tz() const { return _this->sets.tz; };
    const char * token() const { return _this->sets.token; };
    const char * buttonHeader() const { return _this->sets.button.header; };
    const char * buttonName() const { return _this->sets.button.name; };
    const char * buttonReport() const { return _this->sets.button.report; };
    GetT( BotSettings::Settings* t){
          this->_this = t;
       };
    private:
    const BotSettings::Settings* _this;   
    } get;
    struct SetT {
    void adminId( BotSettings::Settings* _this, long long id) {  _this->sets.adminId = id; };
    void chatId( BotSettings::Settings* _this, long long id) { _this->sets.chatId = id; };
    void tz( BotSettings::Settings* _this, int tz) { _this->sets.tz = tz; };
    void token( BotSettings::Settings* _this, const char* token ) { strcpy(_this->sets.token, token); };
    void buttonHeader( BotSettings::Settings* _this, const char* header)  { strcpy(_this->sets.button.header, header); };
    void buttonName( BotSettings::Settings* _this, const char* name)  { strcpy( _this->sets.button.name, name); };
    void buttonReport( BotSettings::Settings* _this, const char* report)  { strcpy( _this->sets.button.report, report); };
    } set;
*/
    size_t printTo(Print& p) const {
        size_t size=0;
        size += p.print(F("Settings '"));
        size += p.print(this->fileName);
        size += p.print(F("'\nAdmin id:")); p.println(this->sets.adminId);
        size += p.print(F("Chat id:")); p.println(this->sets.chatId);
        size += p.print(F("Token:")); p.println(this->sets.token);
        size += p.print(F("Tz:")); p.println(this->sets.tz);
        size += p.print(F("Button header:")); p.println(this->sets.button.header);
        size += p.print(F("Button name:")); p.println(this->sets.button.name);
        size += p.print(F("Button report:")); p.println(this->sets.button.report);
        return size;
    };
  };

}