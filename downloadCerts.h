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

namespace CertStoreFiles {
    static const char dataCerts[] PROGMEM = "data/certs.ar";
    ///"certs.ar"
    static const char * fileData PROGMEM = dataCerts+4; //"/certs.ar";
    static const char fileIdx[] PROGMEM = "/certs.idx";
};
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

    int strFind(const char * srs, char find, const int start = 0){
    int pos = start;
    while( srs[pos] != '\0' ){
        if ( srs[pos] == find ) return pos;
        pos++;
    }
    return -1;
    };
    int _mounth(su::Text& str){
    switch( str.hash() ){
        case "Jan"_h: return 0;
        case "Feb"_h: return 1;
        case "Mar"_h: return 2;
        case "Apr"_h: return 3;
        case "May"_h: return 4;
        case "Jul"_h: return 5;
        case "Jun"_h: return 6;
        case "Aug"_h: return 7;
        case "Sep"_h: return 8;
        case "Oct"_h: return 9;
        case "Nov"_h: return 10;
        case "Dec"_h: return 11;
        default: return -1;
    };
    }
    int _mounth(const char* str){
    su::Text _str(str,3);
    return _mounth(_str);
    }

    time_t getDate(const char * str){
        static const char format[] PROGMEM = ", %d %s %4d %2d:%2d:%2d ";
        time_t out = 0;
        if ( str == nullptr || str[0] == '\0' ) return out;
        int pos = strFind(str, ',');
        if ( pos < 0 ) return out;
        const char * dateString = str + pos;
        
        {
        char mountBuf[4] = {0};
        struct tm timeinfo;
        auto args = sscanf(dateString, format, &timeinfo.tm_mday, mountBuf, &timeinfo.tm_year, &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec );
        debugPrint(args);

        if ( args == 6 ){
            timeinfo.tm_mon = _mounth( mountBuf );
        debugPrintf(" ==> Date: %2u-%02u-%4u Time: %2u:%02u:%02u\n",
            timeinfo.tm_mday, timeinfo.tm_mon+1, timeinfo.tm_year,
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec );

            tmElements_t tmSet;
            tmSet.Year = timeinfo.tm_year-1970;
            tmSet.Month = timeinfo.tm_mon;
            tmSet.Day = timeinfo.tm_mday;
            tmSet.Hour = timeinfo.tm_hour;
            tmSet.Minute = timeinfo.tm_min;
            tmSet.Second = timeinfo.tm_sec;
            
            out = makeTime(tmSet);
            debugPrintln( out );
        } else {
            debugPrintln(F(" Error"));
        }
        }
        return out;
    }
    inline time_t getDate(const String& dateString){
        return getDate(dateString.c_str());
    }

    Errors download(FS& fs, const char * fileName = CertStoreFiles::fileData ){
        //bool res = false;
        debugPretty;
        if ( ! fs.begin() ) return Errors::noFs;

        WiFiClientSecure client;

        time_t dateCertStore = 0;
        int storeSize = 0;

        int numCerts = 0;
        if ( fs.exists( fileName ) ){
            {
                auto csFile = fs.open( fileName, "r" );
                if ( csFile ){
                    dateCertStore = csFile.getLastWrite();
                    storeSize = csFile.size();
                    debugPrintf("%s size=%d, modificated ", fileName, storeSize );
                    debugPrintln( Time::toStr(dateCertStore) );
                }
                csFile.close();
            }
            // if ( certStore == nullptr ) {
            //     certStore = new CertStore();
            // }
            // // numCerts = certStore->initCertStore(fs, CertStoreFiles::fileIdx, fileName );
            // if ( numCerts > 0 )
            //     client.setCertStore( certStore );
            // else {
            //     delete[] certStore;
            //     certStore = nullptr;
            // }
        //client.setFingerprint(App::GITHUB_IO_FINGERPRINT);
        }
        if ( numCerts == 0 ) {
            client.setInsecure();
            debugPrintf("Client connected to %s with fingerprint checking\n", App::gitHubUserContentHost );
        } else {
            debugPrintf("Client connected to %s with %d certs store checking\n", App::gitHubUserContentHost, numCerts );
        }

#ifdef MFLN_SIZE
        bool mfln = client.probeMaxFragmentLength( App::gitHubUserContentHost, App::gitHubPort, MFLN_SIZE);
        if (mfln) {
            client.setBufferSizes(MFLN_SIZE, MFLN_SIZE);
            debugPrintln(F("Set mfln size 1024"));
        }
#endif
        client.setTimeout(1500);

        // if (! client.connect( App::gitHubUserContentHost, App::gitHubPort) ) return Errors::noConnect;

        const char * tmpFile = fileName +3;
        if ( fs.exists( tmpFile) ) fs.remove( tmpFile);
        auto file = fs.open(tmpFile, "w+"); 
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

        if ( httpCode == HTTP_CODE_OK ) {
        
        //int length = http.getSize();
        
        #ifdef debug_print
            debugPrintln(F("Collected headers:"));
            for( int h=0; h<http.headers(); h++){
            debugPrintln( http.header(h));
            }
            debugPrintln(F("==================="));
        #endif


        // time_t dateGitHubCertStore = getDate( http.header(headerDate) );

        // bool needUpdateCerts  = ( dateGitHubCertStore > dateCertStore );
        // debugPrintf("My store date: %lld, GitHub store date: %lld\nNeed update: %s\n", dateCertStore, dateGitHubCertStore,
        // ( needUpdateCerts ? F("yes") : F("no"))
        //  );

        // if ( ! needUpdateCerts ) {
        //     debugPrintf("%s < %s\n", Time::toStr(dateGitHubCertStore), Time::toStr( dateCertStore ));
        //     http.end(); 
        //     file.close();
        //     return noNewStore;
        // }

        if ( http.getSize() == storeSize ){
            debugPrintln("No update needed");
            http.end();
            file.close();
            return noNewStore;
        } else {
            debugPrintf("My store size: %d, GitHub store size: %d, Need update\n", storeSize, http.getSize() );
            
        }
    
        bool contentOctetStream = http.header(content).endsWith(F("octet-stream"));
        if ( ! contentOctetStream || http.getSize() == 0 ){
            debugPrintln(  http.header(content));
            debugPrintf("No Content: octed=%s, len = %d\n", contentOctetStream ? "true":"false", http.getSize() );
            http.end();
            file.close();
            return Errors::noContent;
        }
        
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
                        builtInLed.toggle();
                    } 
                    delay(0);
                }
            }

            builtInLed.off();
         } else {
            debugPrintf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());

        }
        http.end();
        file.close();
        
        if( httpCode != HTTP_CODE_OK ) return Errors::notFound;

        if ( client.connected() )
            client.stop();

        debugPrintf("Writed %d bytes from %u\n", writed, http.getSize() );
        if ( writed == http.getSize() || ( http.getSize() == -1 && writed ) ) {
            if ( fs.rename( tmpFile, fileName ))
                return Errors::ok;
        }
        //}
        return Errors::errorFile;
    };
}