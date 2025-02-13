#pragma once
#include <PairsFile.h>

namespace WiFiPower {
     
    static const char powerFile[] PROGMEM = "/power.dat";
    static const char* data = powerFile+7;
    class WiFiPower : PairsFile {
        private:
            const float test = ( 20.5 * 100 ) /20.5;
            
            uint8_t toPercent(float p) const {
                return (uint8_t)(( p * 100 )/ 20.5);
            };
            float toDb(uint8_t percent) const {
                return ( 20.5 * percent ) / 100;
            };
        public:
        bool read(){ return begin(); };
        bool isWrited(){
            return tick();
        };
        // String writer(){ 
        //     String msg;
        //     if ( tick() ){
        //        msg = F("Wifi power writed");
        //     } 
        //     return msg;
        // };
        WiFiPower(fs::FS* fs ) :
            PairsFile(fs, powerFile ){};
        /**
         * set the output power of WiFi
         * @param percent 0-100% 
         * @note max: +20.5dBm  min: 0dBm
         */             
        uint8_t setPower(uint8_t percent=0){
            uint8_t out;
            float power;
            if( percent == 0 ){
                if( has(data)){
                    power = get( data );
                } else {
                    power = WIFI_POWER;
                }
                out = toPercent( power );
                WiFi.setOutputPower( power );
            } else {
                power = toDb(percent);
                debugPrint("Set db=");
                debugPrintln(power);
                if ( set( data, power )){
                    WiFi.setOutputPower( power );
                    out = percent;
                } else
                    debugPrintln( F("No power setted"));
            }
            return out;
        };
        uint8_t getPower(){
            float power;
            if( has(data)){
                power = get( data );
            } else {
                power = WIFI_POWER;
            }
            return toPercent( power );
        };
    };
    WiFiPower wifiPower( &LittleFS );
};
//PairsFile menuIds(&LittleFS, "/menu.dat", 3000);
namespace MenuIdsNames {
    static const char menu_dat[] PROGMEM = "/menu.dat";
    static const char ignore[] PROGMEM = "ignore";
};

class MenuIds : public PairsFile {
    public:
    MenuIds(fs::FS* nfs = nullptr, const char* path = nullptr, uint32_t tout = 10000) : 
        PairsFile(nfs, path, tout)
        {};
    bool setMenuId(const long long key, const pairs::Value& val){
        //Value _key(key);
        return set( Value(key).str(), val);
    };
    bool hasMenuId(const long long key)  { 
        return has(Value(key).str());
    };
    Pair getMenuId(const long long key)  {
        return get( Value(key).str());
    };
    bool removeMenuId(const long long key){      
        return remove( Value(key).str() );
    };
    bool setChannelName(const long long key, const pairs::Value& val){
        // String _key('n');
        // _key += Value(key).c_str();
        // return set( _key, val);
        return set( strChannelName(key), val );
    };
    Pair getChannelName(const long long key)  {
        // String _key('n');
        // _key += Value(key).c_str();
        // return get( _key);
        return get( strChannelName(key));
    };
    bool hasChannelName(const long long key)  {
        return has( strChannelName(key));
    };
    bool removeChannelName(const long long key){
        // String _key('n');
        // _key += Value(key).c_str();
        // return remove( _key );
        return remove( strChannelName(key));
    };

    bool setUpgradeId(const long long key, const pairs::Value& val){
        // String _key('u');
        // _key += Value(key).c_str();
        // return set( key, val);
        return set( strUpgrade(key) , val);
    };
    Pair getUpgradeId(const long long key)  {
        // String _key('u');
        // _key += Value(key).c_str();
        // return get( _key);
        return get( strUpgrade(key));
    };
    bool removeUpgradeId(const long long key){
        // String _key('u');
        // _key += Value(key).c_str();
        // return remove( _key );
        return remove( strUpgrade( key) );
    };
    Pair getIgnoreVersion(){
        return get(MenuIdsNames::ignore);
    };
    bool hasIgnoreVersion(){
        return has(MenuIdsNames::ignore);
    };
    bool removeIgnoreVersion(){
        bool res = true;
        if ( hasIgnoreVersion() ){
            res = remove(MenuIdsNames::ignore);
        }
        return res;
    };
    bool setIgnoreVersion(const String& ver)
    {   
        if( ver.isEmpty() ){
            return remove(MenuIdsNames::ignore);
        }
        return set(MenuIdsNames::ignore, ver);
    };
    private:
     
    const String str(const long long key, const char prefix='\0' ) const {
        String out(prefix);
        out += Value(key).c_str();
        return out;
    };
    inline const String strUpgrade(const long long key) const { return str( key, 'u'); }
    inline const String strChannelName(const long long key) const { return str( key, 'n'); }
};

MenuIds menuIds(&LittleFS, MenuIdsNames::menu_dat, 3000);
// PairsFile pairsFile;
// pairsFile.set()