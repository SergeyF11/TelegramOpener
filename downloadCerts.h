#pragma once

#include <WiFiClientSecure.h>
#include <LittleFS.h>
#include <ESP8266HTTPClient.h>
//#include <ESP_OTA_GitHub.h>
#include "env.h"
#include "github_upgrade.h"

//#include "myFastBotClient.h"
//#include <ESP8266HTTPClient.h>

namespace CertStoreFiles {
    const char fileData[] PROGMEM = "/certs.ar";
    const char fileIdx[] PROGMEM = "/certs.idx";
};
namespace CertificateStore { 
    enum Errors {
        ok = 0,
        noFs,
        noConnect,
        errorFile,
        notFound,
    };
    namespace myLink {
    //static const char proto[] PROGMEM = "https://";
    static const char host[] PROGMEM = "raw.githubusercontent.com";
    //static const char path[] PROGMEM = "/SergeyF11/TelegramOpener/refs/heads/main/data/certs.ar";
    static const char path[] PROGMEM = "/refs/heads/main/data/certs.ar";
    const int port = 443;
    };
    
    Errors download(FS& fs, const char * fileName = CertStoreFiles::fileData ){
        //bool res = false;
        debugPretty;
        if ( ! fs.begin() ) return Errors::noFs;

        WiFiClientSecure client;
        //client.setInsecure();
#if defined GITHUB_CERTIFICATE_ROOT and defined GITHUB_CERTIFICATE_ROOT1        
        X509List certsList;
        certsList.append(GITHUB_CERTIFICATE_ROOT);
        certsList.append(GITHUB_CERTIFICATE_ROOT1);
        client.setTrustAnchors(&certsList);
#else
        client.setInsecure();
#endif
        bool mfln = client.probeMaxFragmentLength( myLink::host, myLink::port, 1024);
        if (mfln) {
            client.setBufferSizes(1024, 1024);
        }
        client.setTimeout(1500);

        if (! client.connect( myLink::host, myLink::port) ) return Errors::noConnect;

#if defined GITHUB_CERTIFICATE_ROOT and defined GITHUB_CERTIFICATE_ROOT1        
        debugPrintf("Client connected to %s with %d certificates\n", myLink::host, certsList.getCount());
#else
        debugPrintf("Client connected to %s with insecure connection\n", myLink::host );
#endif
                // String url( Url::_preSlash(Author::gitHubAka) ); 
                // url += Url::_preSlash( App::name );
                // url += myLink::path;
                // HTTPClient http;
                // http.setTimeout(8000);

                // if ( http.begin(client, myLink::host, myLink::port, url, /*https=*/true )){
                    
                //     int httpCode = http.GET();
                //     debugPretty;
                //     debugPrintf("Get %s:%d %s\n\tResult: %d\n", myLink::host, myLink::port, url.c_str(), httpCode);
                    
                //     if ( httpCode == HTTP_CODE_OK ){
                      

    //    ESPOTAGitHub::_HTTPget(client, myLink::host, Author::gitHubAka, App::name ,myLink::path, nullptr );
    client.print(F("GET ")); /* client.print(myLink::proto );
    client.print( myLink::host ); */ 
    client.print( Url::_preSlash( Author::gitHubAka));
    client.print( Url::_preSlash( App::name ));
    client.print(myLink::path);
    client.print(F(" HTTP/1.1\r\n"));
    client.print(F("Host: ") ); client.println( myLink::host );
    client.println(F("User-Agent: ESP8266"));
    client.println(F("Connection: close\r\n"));

    const char * tmpFile = fileName +3;
    if ( fs.exists( tmpFile) ) fs.remove( tmpFile);
    auto file = fs.open(tmpFile, "w+"); 
    if( ! file ) return Errors::errorFile;
    
                        // int len = http.getSize();
                        // int length = len;
                        // debugPrintf("Length:%d\n", len);

                        // uint8_t buff[1024] = { 0 };
                        // size_t writed = 0;

                        // auto stream = http.getStream();

                        // size_t size = 1;

                        //     if (size)
                        //     {
                        //         // read up to 1024 byte
                        //         int c = client.readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

                        // while ( len > 0 || ( len == -1 && size > 0  )){
                        // //     // get available data size
                        //     size_t size = stream.available();
                        //     digitalWrite( BUILTIN_LED, !digitalRead( BUILTIN_LED));
                        //     if (size)
                        //     {
                        //         // read up to 1024 byte
                        //         int c = stream.readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                        //         //debugPrintf("Availible %d, readed %d\n", size, c);
                                
                        //         // write it to file
                        //         writed += file.write(buff, c);

                        //         if (len > 0)
                        //         {
                        //             len -= c;
                        //         }
                        //     }
                        //     delay(10);
                        // }
                        // delay(1);
                        // file.close();
                    
                        
        int len;
        int httpCode;
        String httpError;

        while (client.connected()) {
            String response = client.readStringUntil('\n');
            //debugPrintln(response);
            if ( response.substring(0,15).equalsIgnoreCase(F("content-length:"))) {
	          	len = response.substring(15).toInt();
	        }
            if ( response.startsWith(F("HTTP/1.1")) ){
                
                httpCode = response.substring(9).toInt();
                httpError += response.substring(13);
            }
	        if (response == "\r") {
	        	break;
	  		}
        }
        if ( httpCode != 200) {
            debugPrintf("Error: %d %s\n", httpCode, httpError);
            return Errors::notFound;
        } else {
            debugPrintf("Response: %d %s\n", httpCode, httpError);
        }
        uint8_t buff[1024] = { 0 };
        size_t writed = 0;
        int length = len;
        /* if ( len == -1 ) */{ client.setTimeout(8000); }
        // (bool *)(unsigned long) timeOut = [](unsigned long start=0){
        //     if ( start != 0 ){
        //         static unsigned long startMs = start;
        //         return true;
        //     } else {
        //         return millis()-startMs >= start;
        //     }
        // };
        // timeOut(millis()); 
        
        // auto ledTogle = [](int period){
        //     static uint8_t status = HIGH;
        //     static auto changeMs = millis();
        //     if ( millis() - changeMs >= period ){
        //         status = ! status;
        //         digitalWrite( BUILTIN_LED, status );
        //         changeMs = millis();
        //     }
        // }; 
        auto ledTogle = [](void){
            static uint8_t status = HIGH;
            digitalWrite( BUILTIN_LED, status );
            status = ! status;
        };

        while (  len > 0 || ( len == -1 && client.connected() /* && ! timeOut() */ ) )
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
                //digitalWrite( BUILTIN_LED, !digitalRead( BUILTIN_LED));
                ledTogle();
            } /* else {
                delay(10);
            } */
            delay(0);
        }
        delay(1);
        file.close();
        if ( client.connected() )
            client.stop();
        debugPrintf("Writed %d bytes from %u\n", writed, length );
        if ( writed == length || ( length == -1 && writed ) ) {

            fs.rename( tmpFile, fileName );
        
            return Errors::ok;
        }
        return Errors::errorFile;
    //};
        //         } else {
        //             return Errors::noConnect;
        //         }
        // return Errors::errorFile;
    };

}