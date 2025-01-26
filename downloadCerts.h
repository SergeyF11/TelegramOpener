#include "relay.h"
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
    };
    // bool getHeader(const char * header, String& s){
    //     int headerLen = strlen(header);
    //     if ( s.substring(0,headerLen).equalsIgnoreCase( header) ){
    //         s = s.substring(headerLen+1);
    //         s.trim();
    //         s.toLowerCase();
    //         debugPrintf("Header: %s , value: \'%s\'\n", header, s.c_str());
    //         return true;
    //     }
    //     return false;
    // };

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
        client.setFingerprint(App::GITHUB_IO_FINGERPRINT);
        //client.setInsecure();
#endif

#ifdef MFLN_SIZE
        bool mfln = client.probeMaxFragmentLength( App::gitHubUserContentHost, App::gitHubPort, MFLN_SIZE);
        if (mfln) {
            client.setBufferSizes(MFLN_SIZE, MFLN_SIZE);
        }
#endif
        client.setTimeout(1500);

        // if (! client.connect( App::gitHubUserContentHost, App::gitHubPort) ) return Errors::noConnect;

#if defined GITHUB_CERTIFICATE_ROOT and defined GITHUB_CERTIFICATE_ROOT1        
        debugPrintf("Client connected to %s with %d certificates\n", myLink::host, certsList.getCount());
#else
        debugPrintf("Client connected to %s with fingerprint checking\n", App::gitHubUserContentHost );
#endif

        const char * tmpFile = fileName +3;
        if ( fs.exists( tmpFile) ) fs.remove( tmpFile);
        auto file = fs.open(tmpFile, "w+"); 
        if( ! file ) {
            debugPrintln(F("File error"));
            return Errors::errorFile;
        }

        debugPrintMemory;
        runStart;

        // client.println( String(F("GET ")) + App::getRawContent( CertStoreFiles::dataCerts, /*host=*/false) +F(" HTTP/1.1\r\n"
        //                 "Host: ") + App::gitHubUserContentHost + F("\r\n"
        //                 "User-Agent: ESP8266\r\n"
        //                 "Connection: close\r\n"));

        // printRunTime;
        // debugPrintMemory;

        // int len = -1;
        // int httpCode;
        // bool contentOctetStream = false;
        
        // {
        //     String httpError;
        //     while (client.connected()) {
        //         String response = client.readStringUntil('\n');
        //         //debugPrintln(response);
        //         if ( getHeader( PSTR("content-type"), response )){
        //             if( response.endsWith(F("octet-stream")))
        //                 contentOctetStream = true;
        //         }
        //         if ( getHeader( PSTR("content-length"), response )) {
        //             len = response.toInt();
        //         }
        //         if ( response.startsWith(F("HTTP/1.1")) ){
                    
        //             httpCode = response.substring(9).toInt();
        //             httpError += response.substring(13);
        //         }
        //         if (response == "\r") {
        //             break;
        //         }
        //     }
        //     if ( httpCode != 200) {
        //         debugPrintf("Error: %d %s\n", httpCode, httpError);
        //         return Errors::notFound;
        //     } else {
        //         debugPrintf("Response: %d %s\n", httpCode, httpError);
        //     }
        // }
        
        // if ( ! contentOctetStream || len == 0 ){
        //     debugPrintf("No Content: octed=%s, len = %d\n", contentOctetStream ? "true":"false", len );
        //     return Errors::noContent;
        // } else {
            
        //     size_t writed = 0;
        //     int length = len;
        //     if ( len == -1) {
        //         client.setTimeout(8000);
        //     }
        //     {
        //         uint8_t buff[512] = { 0 };
        //         while (  len > 0 || ( len == -1 && client.connected() /* && ! timeOut() */ ) )
        //         {
        //             // get available data size
        //             size_t size = client.available();

        //             if (size)
        //             {
        //                 // read up to 1024 byte
        //                 int c = client.readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

        //                 // write it to file
        //                 writed += file.write(buff, c);

        //                 if (len > 0)
        //                 {
        //                     len -= c;
        //                 }
        //                 builtInLed.toggle();
        //             } 
        //             delay(0);
        //         }
        //     }
        //     delay(1);
            HTTPClient http;
            if ( ! http.begin( client,  App::gitHubUserContentHost, App::gitHubPort, App::getRawContent( CertStoreFiles::dataCerts, false), true ) ){
                http.end(); 
                debugPrintln(F("Connection error"));
                return Errors::noConnect;
            }
            int writed=0;
            static const char content[] PROGMEM = "content-type";
            //const char * headers[] = { content };
            const char * headers[] = { content };
            http.collectHeaders( headers ,1);
            int httpCode = http.GET();
            //if (httpCode > 0) {
            int length = http.getSize();

            #ifdef debug_print
             for( int h=0; h<http.headers(); h++){
                debugPrintln( http.header(h));
             }
            #endif
            bool contentOctetStream = http.header(content).endsWith(F("octet-stream"));
            if ( ! contentOctetStream || length == 0 ){
                debugPrintln(  http.header(content));
                debugPrintf("No Content: octed=%s, len = %d\n", contentOctetStream ? "true":"false", length );
                http.end();
                return Errors::noContent;
            }
            if ( httpCode == HTTP_CODE_OK ) {
                int len = length;
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
                //delay(1);
                //builtInLed.on();
                //writed = http.writeToStream(&file);
                builtInLed.off();
        //    }
            } else {
                debugPrintf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
                // http.end();
                // file.close();
                // return Errors::notFound;
            }
            http.end();
            file.close();
            
            if( httpCode != HTTP_CODE_OK ) return Errors::notFound;

            if ( client.connected() )
                client.stop();

            debugPrintf("Writed %d bytes from %u\n", writed, length );
            if ( writed == length || ( length == -1 && writed ) ) {
                if ( fs.rename( tmpFile, fileName ))
                    return Errors::ok;
            }
        //}
        return Errors::errorFile;
    };
}