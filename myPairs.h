#pragma once
#include <PairsFile.h>

//PairsFile menuIds(&LittleFS, "/menu.dat", 3000);

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

    private:
    String str(const long long key, const char prefix='\0' ) const {
        String out(prefix);
        out += Value(key).c_str();
        return out;
    };
    inline String strUpgrade(const long long key) const { return str( key, 'u'); }
    //String strMenu(const long long key){ return str( key, 'm'); }
    inline String strChannelName(const long long key) const { return str( key, 'n'); }
    
    // bool set(const long long key, const pairs::Value& val){
    //     Value _key(key);
    //     //_key += key;
    //     return PairsFile::set( _key.str() , val);
    // };
    // bool set(const char prefix,  const long long key, const pairs::Value& val){
    //     String _key;
    //     _key += prefix;
    //     _key += Value(key).c_str();
    //     return PairsFile::set( _key, val);
    // };
    // Pair get(const long long key ){
    //     Value _key(key);
    //     //_key += key;
    //     return PairsFile::get( _key.str() );
    // };
    // Pair get(const char prefix,  const long long key){
    //     String _key;
    //     _key += prefix;
    //     _key += Value(key).c_str();
    //     return PairsFile::get( _key );
    // };
    // bool remove( const long long key ){
    //     Value _key(key);
    //     //_key += key;
    //     return PairsFile::remove( _key.str() );
    // };
    // bool remove(const char prefix,  const long long key){
    //     String _key;
    //     _key += prefix;
    //     _key += Value(key).c_str();
    //     return PairsFile::remove( _key );
    // };
};

MenuIds menuIds(&LittleFS, "/menu.dat", 3000);
// PairsFile pairsFile;
// pairsFile.set()