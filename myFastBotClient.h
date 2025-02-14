#pragma once

#include <FastBot2Client.h>
#include <LittleFS.h>
#include <WiFiClientSecure.h>
#include "downloadCerts.h"

CertStore* certStore = nullptr;
WiFiClientSecure client;
FastBot2Client bot(client);


// namespace Telegram {
//     static const char fingerprint[] PROGMEM = "1F:77:5F:20:C5:D3:BD:67:DE:E8:07:9B:59:1D:22:E9:C0:E4:52:4B"; //api.telegram.org
// };
bool botCertsStore(CertStore* cs, WiFiClientSecure& cl, FS& fs, const char * fileData=CertStoreFiles::fileData){
    if ( cs != nullptr ) { 
        delete[](cs);
        cs == nullptr;
    }
    int numCerts = 0;
    if ( fs.begin() ) {
        if ( fs.exists( CertificateStore::TmpFile::fileName )){
            auto f = fs.open( CertificateStore::TmpFile::fileName, "r");
            bool needRename = ( f.size() != 0 && CertStoreFiles::hasNewestCertsStore() );
            f.close();
            if ( needRename ){
                fs.rename( CertificateStore::TmpFile::fileName, fileData );
                debugPrintln("CertStore Upgraded");
            } else {
                fs.remove( CertificateStore::TmpFile::fileName );
                debugPrintf("Tmp file %s deleted\n", CertificateStore::TmpFile::fileName);
            }
        }
        if ( ! fs.exists(fileData)  ) {
            cl.setInsecure();
            if ( CertificateStore::insecureDownload(LittleFS) != CertificateStore::Errors::ok )
            return 0;
        } else {

        // }
        // if ( fs.exists(fileData) || 
        // //  надо  будет заменить на update
        //     // CertificateStore::download(LittleFS) == CertificateStore::Errors::ok ) {
        //     CertificateStore::insecureDownload(LittleFS) == CertificateStore::Errors::ok ) {
            
            if ( cs == nullptr ){
                cs = new CertStore();
            }
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

