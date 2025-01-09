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
    const char dataCerts[] PROGMEM = "data/certs.ar";
    ///"certs.ar"
    const char * fileData = dataCerts+4; //"/certs.ar";
    const char fileIdx[] PROGMEM = "/certs.idx";
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
        bool mfln = client.probeMaxFragmentLength( App::gitHubUserContentHost, App::gitHubPort, 1024);
        if (mfln) {
            client.setBufferSizes(1024, 1024);
        }
        client.setTimeout(1500);

        if (! client.connect( App::gitHubUserContentHost, App::gitHubPort) ) return Errors::noConnect;

#if defined GITHUB_CERTIFICATE_ROOT and defined GITHUB_CERTIFICATE_ROOT1        
        debugPrintf("Client connected to %s with %d certificates\n", myLink::host, certsList.getCount());
#else
        debugPrintf("Client connected to %s with fingerprint checking\n", App::gitHubUserContentHost );
#endif

        const char * tmpFile = fileName +3;
        if ( fs.exists( tmpFile) ) fs.remove( tmpFile);
        auto file = fs.open(tmpFile, "w+"); 
        if( ! file ) return Errors::errorFile;

    debugPrintMemory;
    runStart;

    client.println( String(F("GET ")) + App::getRawContent( CertStoreFiles::dataCerts, /*host=*/false) +F(" HTTP/1.1\r\n"
                    "Host: ") + App::gitHubUserContentHost + F("\r\n"
                    "User-Agent: ESP8266\r\n"
                    "Connection: close\r\n"));
    // client.println( String(F("GET "
    //                 "/")) + Author::gitHubAka + "/" + App::name + myLink::path +F(" HTTP/1.1\r\n"
    //                 "Host: ") + myLink::host + F("\r\n"
    //                 "User-Agent: ESP8266\r\n"
    //                 "Connection: close\r\n"));
    printRunTime;
    debugPrintMemory;

        int len = -1;
        int httpCode;
        String httpError;
        bool contentOctetStream = false;

        auto getHeader = [](const char * header, String& s){
            int headerLen = strlen(header);
            if ( s.substring(0,headerLen).equalsIgnoreCase( header) ){
                s = s.substring(headerLen+1);
                s.trim();
                s.toLowerCase();
                debugPrintf("Header: %s , value: \'%s\'\n", header, s.c_str());
                return true;
            }
            return false;
        };

        while (client.connected()) {
            String response = client.readStringUntil('\n');
            //debugPrintln(response);
            if ( getHeader( PSTR("content-type"), response )){
                if( response.endsWith(F("octet-stream")))
                    contentOctetStream = true;
            }
            if ( getHeader( PSTR("content-length"), response )) {
	          	len = response.toInt();
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
        if ( ! contentOctetStream || len == 0 ){
            debugPrintf("No Content: octed=%s, len = %d\n", contentOctetStream ? "true":"false", len );
            return Errors::noContent;
        }
        uint8_t buff[512] = { 0 };
        size_t writed = 0;
        int length = len;
        /* if ( len == -1 ) */{ client.setTimeout(8000); }
 
        auto ledTogle = [](void){
            static uint8_t status = LOW;
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

                ledTogle();
            } 
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