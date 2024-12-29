#pragma once

#include <WiFiClientSecure.h>
#include <LittleFS.h>
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
    static const char proto[] PROGMEM = "https://";
    static const char host[] PROGMEM = "raw.githubusercontent.com";
    static const char path[] PROGMEM = "/SergeyF11/TelegramOpener/refs/heads/main/data/data/certs.ar";
    const int port = 443;
    };
    
    Errors download(FS& fs, const char * fileName = CertStoreFiles::fileData ){
        //bool res = false;
        
        if ( ! fs.begin() ) return Errors::noFs;

        WiFiClientSecure client;
        client.setInsecure();
        
        if (! client.connect( myLink::host, myLink::port) ) return Errors::noConnect;
    
    client.print(F("GET ")); client.print(myLink::proto );
    client.print( myLink::host ); client.print(myLink::path);
    client.print(F(" HTTP/1.1\r\n"));
    client.print(F("Host: ") ); client.print( myLink::host );
    client.print(F("\r\n"));
    client.print(F("User-Agent: ESP8266\r\n"));
    client.print(F("Connection: close\r\n\r\n"));
    const char * tmpFile = fileName +3;
        if ( fs.exists( tmpFile)) fs.remove( tmpFile);
        auto file = fs.open(tmpFile, "w+"); 
        if( ! file ) return Errors::errorFile;
        int len = -1;

        while (client.connected()) {
            String response = client.readStringUntil('\n');
            
            if ( response.substring(0,15).equalsIgnoreCase(F("content-length:"))) {
	          	len = response.substring(15).toInt();
	        }
	        if (response == "\r") {
	        	break;
	  		}
        }
        uint8_t buff[128] = { 0 };
        size_t writed = 0;

        while (client.connected() && (len > 0 || len == -1))
        {
            // get available data size
            size_t size = client.available();

            if (size)
            {
                // read up to 128 byte
                int c = client.readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

                // write it to file
                writed += file.write(buff, c);

                if (len > 0)
                {
                    len -= c;
                }
            }
            delay(1);
        }
        file.close();
        if ( client.connected() )
            client.stop();
        if ( writed ) {

            fs.rename( tmpFile, fileName );
        
            return Errors::ok;
        }
        return Errors::errorFile;
    };

}