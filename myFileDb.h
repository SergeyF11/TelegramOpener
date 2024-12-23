#pragma once
#include <GyverDB.h>
#include <GyverDBFile.h>

class MenuIds : public GyverDBFile {
    public:
    MenuIds( fs::FS* nfs = nullptr, const char* path = nullptr, uint32_t tout = 10000) :
        GyverDBFile(nfs , path , tout ){};
    
    bool setMenuId(const long long key, const Value& val){
        //Value _key(key);
        return set( Value(key).str(), val);
    };
    bool hasMenuId(const long long key)  { 
        return has(Value(key).str());
    };
    int32_t getMenuId(const long long key)  {
        return get( Value(key).str()).toInt32();
    };
    void removeMenuId(const long long key){      
         remove( Value(key).str() );
    };
    bool setChannelName(const long long key, const Value& val){
        return set( strChannelName(key), val );
    };
    String getChannelName(const long long key)  {
        return get( strChannelName(key)).toString();
    };
    bool hasChannelName(const long long key)  {
        return has( strChannelName(key));
    };
    void removeChannelName(const long long key){
         remove( strChannelName(key));
    };

    bool setUpgradeId(const long long key, const Value& val){
        return set( strUpgrade(key) , val);
    };
    int32 getUpgradeId(const long long key)  {
        return get( strUpgrade(key)).toInt32();
    };
    void removeUpgradeId(const long long key){
         remove( strUpgrade( key) );
    };

    private:
    String str(const long long key, const char prefix='\0' ) const {
        String out(prefix);
        out += Value(key).c_str();
        return out;
    };
    inline String strUpgrade(const long long key) const { return str( key, 'u'); }
    inline String strChannelName(const long long key) const { return str( key, 'n'); }
};

MenuIds menuIds(&LittleFS, "/menu.bin", 3000);
