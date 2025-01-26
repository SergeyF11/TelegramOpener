#pragma once

#include <FastBot2Client.h>
#include <LittleFS.h>
#include <WiFiClientSecure.h>
#include "downloadCerts.h"

CertStore* certStore = nullptr;
WiFiClientSecure client;
FastBot2Client bot(client);


namespace Telegram {
    static const char fingerprint[] PROGMEM = "1F:77:5F:20:C5:D3:BD:67:DE:E8:07:9B:59:1D:22:E9:C0:E4:52:4B"; //api.telegram.org
};
bool botCertsStore(CertStore* cs, WiFiClientSecure& cl, FS& fs, const char * fileData=CertStoreFiles::fileData){
    if ( cs != nullptr ) delete[](cs);
    int numCerts = 0;
    if ( fs.begin() ) {
        if ( fs.exists(fileData) ||
        CertificateStore::download(LittleFS) == CertificateStore::Errors::ok ) 
        {
            cs = new CertStore();
            numCerts = cs->initCertStore(fs, CertStoreFiles::fileIdx, fileData);
            if ( numCerts > 0 ) {
                cl.setCertStore(cs);    
            } else {
                delete[](cs);
                cs = nullptr;
            }
        } 
    }
    return numCerts;
};
// CertStore* botCertsStore(WiFiClientSecure& cl, FS& fs, const char * fileData=CertStoreFiles::fileData){
//     int numCerts = 0;
//     CertStore * certStore = nullptr;
//     /*if ( certsStore != nullptr){ 
//         numCerts++;
//     } else */
//     if ( fs.begin() ) {
//         if ( fs.exists(fileData) ||
//              CertificateStore::download(LittleFS) == CertificateStore::Errors::ok ) {
        
//             certStore = new CertStore();
//             numCerts = certStore->initCertStore(fs, CertStoreFiles::fileIdx, fileData);
//             if ( numCerts > 0 ) {
//                 cl.setCertStore(certStore);    
//             } else {
//                 delete[](certStore);
//                 certStore = nullptr;
//             }
//         }
//     } 
    
//     if ( numCerts == 0 ) {
//         cl.setFingerprint(Telegram::fingerprint );
//         //numCerts = -1;
//     }
//     return certStore;
// };

