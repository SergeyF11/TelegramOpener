#pragma once
#include <FastBot2.h>
#include "TelegramMD.h"
#include "WiFiManager.h"
#include "debug.h"


extern FastBot2Client bot;
#define IS_SIGNAL_GOOD(DBM) DBM>-30
#define IS_SIGNAL_POOR(DBM) DBM<-90


namespace RSSI {
    volatile long long userID;
    volatile int32_t dBm;
    volatile int networks;
    void begin(const long long id){
        userID =id;
        networks=0;
        needStart = NeedStartE::GetRSSI;
    }

    bool sendReport(){
        bool res = ( networks > 0 );
        if(  res ){
            debugPrintf("'%s' RSSI=%d\n", 
            wm.getWiFiSSID().c_str(), dBm );

            String rssi(dBm);
                rssi += "dBm";
            fb::Message msgRssi;
                msgRssi.setModeMD();
                msgRssi.text = TelegramMD::asBold( wm.getWiFiSSID().c_str() , MARKDOWN_TG::escape );
                msgRssi.text += PSTR("rssi\\=");
                msgRssi.text += (  IS_SIGNAL_GOOD(RSSI::dBm) ) ? 
                    TelegramMD::asBold(rssi, MARKDOWN_TG::escape ) : MARKDOWN_TG::escape( rssi );
                msgRssi.chatID = userID;
            debugPrintln( msgRssi.text );
            bot.sendMessage( msgRssi, false );
        }
        return res;
    }
    
}