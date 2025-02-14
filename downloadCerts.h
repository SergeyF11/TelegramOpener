#include "relay.h"
#pragma once

#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include <ESP8266HTTPClient.h>
//#include <ESP_OTA_GitHub.h>
#include "env.h"
#include "github_upgrade.h"
#include <time.h>
#include <TimeLib.h>
#include <StringUtils.h>

//#include "myFastBotClient.h"
//#include <ESP8266HTTPClient.h>

namespace FileTime 
{
    namespace {
    static time_t fileTime;
    time_t myTimeCallback() {
        return fileTime;
    };
    time_t _defaultTime(){
        return time(nullptr);
    };
    void setFilesTime(const time_t t = 0){
        if ( t ){
            fileTime = t;
            LittleFS.setTimeCallback(myTimeCallback);
        } else {
            LittleFS.setTimeCallback(_defaultTime);
        }
    };
    }
    bool setModificated(FS& fs, const char * file, time_t t){
        bool res = false;
        setFilesTime( t );
        auto _file = fs.open(file, "r+");
        if ( _file ){         
            uint8_t c;
            if ( _file.read(&c,1) ){
                _file.seek(0);
                res = _file.write(&c, 1);
                if ( !res) Serial.println("Error write byte");
            } else {
                if ( !res) Serial.println("Error read byte");
            }
            _file.close();
            
        } else {
            if ( !res) Serial.printf("Error open file %s\n", file );
        }    
        setFilesTime();
        return res;
    }
}

namespace CertificateStore { 
    enum Errors {
        ok = 0,
        noFs,
        noConnect,
        errorFile,
        notFound,
        noContent,
        noNewStore,
    };

    // int strFind(const char * srs, char find, const int start = 0){
    // int pos = start;
    // while( srs[pos] != '\0' ){
    //     if ( srs[pos] == find ) return pos;
    //     pos++;
    // }
    // return -1;
    // };
    // int _mounth(su::Text& str){
    // switch( str.hash() ){
    //     case "Jan"_h: return 0;
    //     case "Feb"_h: return 1;
    //     case "Mar"_h: return 2;
    //     case "Apr"_h: return 3;
    //     case "May"_h: return 4;
    //     case "Jul"_h: return 5;
    //     case "Jun"_h: return 6;
    //     case "Aug"_h: return 7;
    //     case "Sep"_h: return 8;
    //     case "Oct"_h: return 9;
    //     case "Nov"_h: return 10;
    //     case "Dec"_h: return 11;
    //     default: return -1;
    // };
    // }
    // int _mounth(const char* str){
    // su::Text _str(str,3);
    // return _mounth(_str);
    // }

    // time_t getDate(const char * str){
    //     static const char format[] PROGMEM = ", %d %s %4d %2d:%2d:%2d ";
    //     time_t out = 0;
    //     if ( str == nullptr || str[0] == '\0' ) return out;
    //     int pos = strFind(str, ',');
    //     if ( pos < 0 ) return out;
    //     const char * dateString = str + pos;
        
    //     {
    //     char mountBuf[4] = {0};
    //     struct tm timeinfo;
    //     auto args = sscanf(dateString, format, &timeinfo.tm_mday, mountBuf, &timeinfo.tm_year, &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec );
    //     debugPrint(args);

    //     if ( args == 6 ){
    //         timeinfo.tm_mon = _mounth( mountBuf );
    //     debugPrintf(" ==> Date: %2u-%02u-%4u Time: %2u:%02u:%02u\n",
    //         timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year,
    //         timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec );

    //         tmElements_t tmSet;
    //         tmSet.Year = timeinfo.tm_year-1970;
    //         tmSet.Month = timeinfo.tm_mon;
    //         tmSet.Day = timeinfo.tm_mday;
    //         tmSet.Hour = timeinfo.tm_hour;
    //         tmSet.Minute = timeinfo.tm_min;
    //         tmSet.Second = timeinfo.tm_sec;
            
    //         out = makeTime(tmSet);
    //         debugPrintln( out );
    //     } else {
    //         debugPrintln(F(" Error"));
    //     }
    //     }
    //     return out;
    // }
    // inline time_t getDate(const String& dateString){
    //     return getDate(dateString.c_str());
    // }

   
    int streamToFile( WiFiClientSecure& client, HTTPClient& http, fs::File& file, void(*inProgressF)() = nullptr /* builtInLed.toggle */ ){
            int writed=0;
            int len = http.getSize();
            if ( len == -1) {
                client.setTimeout(5000);
            }
            {
                static uint8_t buff[256] = { 0 };
                while (  len > 0 || ( len == -1 && http.connected() /* && ! timeOut() */ ) )
                {
                    // get available data size
                    size_t size = client.available();

                    if (size)
                    {
                        // read up to 1024 byte
                        int c = client.readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

                        // write it to file
                        writed += file.write(buff, c);

                        if (len > 0)
                        {
                            len -= c;
                        }
                        if ( inProgressF ) inProgressF();;
                    } 
                    delay(1);
                }
            }
        return writed;
    };

namespace TmpFile {
    static bool correct=false;
    static constexpr const char * fileName PROGMEM = CertStoreFiles::fileData+3;
    // File open(FS& fs=LittleFS, const char * tmpFile= fileName, const char * mode="w+" ){
    //     if ( fs.exists( tmpFile) ) fs.remove( tmpFile);
    //     return fs.open(tmpFile, mode); 
    // };
    // bool rename(FS& fs=LittleFS, const char * newName, const char * tmpFile=fileName){
    //     return fs.rename( tmpFile, newName );
    // };
}
    
    Errors download(FS& fs, const char * fileName = CertStoreFiles::fileData ){
        //bool res = false;
        debugPretty;
        if ( ! fs.begin() ) return Errors::noFs;

        WiFiClientSecure client;
        client.setInsecure();

#ifdef MFLN_SIZE
        bool mfln = client.probeMaxFragmentLength( App::gitHubUserContentHost, App::gitHubPort, MFLN_SIZE);
        if (mfln) {
            client.setBufferSizes(MFLN_SIZE, MFLN_SIZE);
            debugPrintln(F("Set mfln size 1024"));
        }
#endif
        client.setTimeout(1500);

        // if (! client.connect( App::gitHubUserContentHost, App::gitHubPort) ) return Errors::noConnect;

        // const char * tmpFile = fileName +3;
        if ( fs.exists( TmpFile::fileName ) ) fs.remove( TmpFile::fileName );
        auto file = fs.open(TmpFile::fileName, "w+"); 
        if( ! file ) {
            debugPrintln(F("File error"));
            return Errors::errorFile;
        }

        debugPrintMemory;
        runStart;

        HTTPClient http;
        if ( ! http.begin( client,  App::gitHubUserContentHost, App::gitHubPort, App::getRawContent( CertStoreFiles::dataCerts, false), true ) ){
            http.end(); 
            file.close();
            debugPrintln(F("Connection error"));
            return Errors::noConnect;
        }

        int writed=0;
        
        static const char content[] PROGMEM = "content-type";
        //static const char headerDate[] PROGMEM = "date";
        
        static const char * headers[] PROGMEM = { content };
        //static const char * headers[] PROGMEM = { content, headerDate };
        http.collectHeaders( headers , 1);
        int httpCode = http.GET();
        int length = http.getSize();
        
        if ( httpCode == HTTP_CODE_OK ) {
        
            #ifdef debug_print
                debugPrintln(F("Collected headers:"));
                for( int h=0; h<http.headers(); h++){
                debugPrintln( http.header(h));
                }
                debugPrintln(F("==================="));
            #endif
    
            bool contentOctetStream = http.header(content).equals(GitHubUpgrade::contentType );//endsWith(F("octet-stream"));
            if ( ! contentOctetStream || http.getSize() == 0 ){
                debugPrintln(  http.header(content));
                debugPrintf("No Content: octed=%s, len = %d\n", contentOctetStream ? "true":"false", http.getSize() );
                http.end();
                file.close();
                return Errors::noContent;
            }
            builtInLed.off();
            writed = streamToFile( client, http, file, [](){ builtInLed.toggle(); } );
            builtInLed.off();
        } 
        http.end();
        file.close();
        
        if( httpCode != HTTP_CODE_OK ) {
            debugPrintf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            return Errors::notFound;
        }

        if ( client.connected() )
            client.stop();

        debugPrintf("Writed %d bytes from %u\n", writed, length );
        if ( writed == length || ( length == -1 && writed ) ) {
            if ( fs.rename( TmpFile::fileName, fileName ) )
                return Errors::ok;
        }
        //}
        return Errors::errorFile;
    };
    
    // class Updater {
    //     private:
    //     WiFiClientSecure * cl;
    //     FS * fs;
    //     enum Status {
    //         None,
    //         CheckUrl,
    //         Relocating,
    //         StartDownloading,
    //         Download,
    //         Finishing,
    //     } status;
    //     //String location;
    //     // const char * fileName = CertStoreFiles::fileData 
    //     public:
    //     Status getStatus(){ return status; };

    // перезаписывает файл CertStore Из временного файла
        bool upgrade(const char *from= TmpFile::fileName, const char * to=CertStoreFiles::fileData, FS& fs=LittleFS){ 
            int res = FileTime::setModificated( fs, from, GitHubUpgrade::release.getNewCertStoreDate() );
            res += fs.rename( from, to );
            return res;
        };

        bool update( GitHubUpgrade::Release& release, FS& fs=LittleFS ){
            bool result = false;
            // switch( status){
            //     case Status::None:
                    if (! release.constructed[GitHubUpgrade::Release::Url::CertStore] ) return result;
                    //if ( ! fs ) fs.begin();
                    if ( fs.exists( TmpFile::fileName ) ) fs.remove( TmpFile::fileName );
                    auto file = fs.open(TmpFile::fileName, "w+"); 
                    
                    if( file ) {
                        HTTPClient http;
                        if ( http.begin( 
                            client,                
                            release.constructUrl(GitHubUpgrade::Release::Url::CertStore) ) )
                        {
                            //http.setRedirectLimit(1);
                            http.setFollowRedirects( HTTPC_FORCE_FOLLOW_REDIRECTS );
                            int code = http.GET();
                            switch( code ){
                                case HTTP_CODE_OK:
                                // dowload here
                                // get content
                                {
                                    int writed = CertificateStore::streamToFile( client, http, file , [](){ builtInLed.toggle(); } );
                                    result = ( writed == http.getSize() );
                                }
                                break;
                                case HTTP_CODE_FOUND:
                                case HTTP_CODE_PERMANENT_REDIRECT:
                                case HTTP_CODE_TEMPORARY_REDIRECT:
                                // redirect     
                                    debugPretty;
                                    debugPrintf("Http code=%d\n", code);
                                
                                break;
                                default:
                                //errors
                                    debugPretty;
                                    debugPrintf("Http code=%d\n", code);
                            }
                            
                        }
                        http.end();
                        file.close();
                    }
                    
                    // if ( result ){
                    //     fs.rename( TmpFile::fileName,  CertStoreFiles::fileData );
                    //     //release.resetCertStoreDate();
                    // }
            //     break;
            // }
            
            return result;
        };
    //     Updater(WiFiClientSecure& client, FS& fs ) :
    //     cl(&client), fs(&fs), status(Status::None)
    //     {};

    // };
    Errors insecureDownload(FS& fs, const char * fileName = CertStoreFiles::fileData ){
        //bool res = false;
        debugPretty;
        if ( ! fs.begin() ) return Errors::noFs;

        // WiFiClientSecure client;
        // client.setInsecure();

        GitHubUpgrade::check(true); // ) return Errors::noContent;
        if ( GitHubUpgrade::release.getNewCertStoreDate() == 0 ) return Errors::noContent;
        debugPrintf("Check Github Ok\n");
        if ( ! update(GitHubUpgrade::release )) return Errors::noContent;
        debugPrintf("Update done: %s\n", GitHubUpgrade::Error().c_str() );
        //GitHubUpgrade::Error() == GitHubUpgrade::Errors::Ok
        if ( ! upgrade() ) return Errors::errorFile;
        return Errors::ok;
    }
}