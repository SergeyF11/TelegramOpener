#pragma once

#include <FastBot2Client.h>
#include <LittleFS.h>
#include <WiFiClientSecure.h>

CertStore* certStore = nullptr;
WiFiClientSecure client;
FastBot2Client bot(client);

namespace CertStoreFiles {
    const char fileData[] PROGMEM = "/certs.ar";
    const char fileIdx[] PROGMEM = "/certs.idx";
};
namespace Telegram {
    const char fingerprint[] PROGMEM = "1F:77:5F:20:C5:D3:BD:67:DE:E8:07:9B:59:1D:22:E9:C0:E4:52:4B"; //api.telegram.org
};

int botCertsStore(WiFiClientSecure& cl, FS& fs, const char * fileData=CertStoreFiles::fileData){
    int numCerts = 0;
    /*if ( certsStore != nullptr){ 
        numCerts++;
    } else */
    if ( fs.begin() && fs.exists(fileData) ){
        certStore = new CertStore();
        numCerts = certStore->initCertStore(fs, CertStoreFiles::fileIdx, fileData);
        if ( numCerts > 0 ) {
            cl.setCertStore(certStore);    
        } else {
            delete(certStore);
            certStore = nullptr;
        }
    } else {
        cl.setFingerprint(Telegram::fingerprint );
        numCerts = -1;
    }
    return numCerts;
};