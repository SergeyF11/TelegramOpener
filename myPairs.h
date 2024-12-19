#pragma once
#include <PairsFile.h>

//PairsFile menuIds(&LittleFS, "/menu.dat", 3000);

class MenuIds : public PairsFile {
    public:
    MenuIds(fs::FS* nfs = nullptr, const char* path = nullptr, uint32_t tout = 10000){
    };

    bool set(const long long key, const pairs::Value& val){
        Value _key(key);
        //_key += key;
        return PairsFile::set( _key.str() , val);
    };
    bool set(const char prefix,  const long long key, const pairs::Value& val){
        String _key;
        _key += prefix;
        _key += Value(key).c_str();
        return PairsFile::set( _key, val);
    };
    Pair get(const long long key ){
        Value _key(key);
        //_key += key;
        return PairsFile::get( _key.str() );
    };
    Pair get(const char prefix,  const long long key){
        String _key;
        _key += prefix;
        _key += Value(key).c_str();
        return PairsFile::get( _key );
    };
    bool remove( const long long key ){
        Value _key(key);
        //_key += key;
        return PairsFile::remove( _key.str() );
    };
    bool remove(const char prefix,  const long long key){
        String _key;
        _key += prefix;
        _key += Value(key).c_str();
        return PairsFile::remove( _key );
    };
};

MenuIds menuIds(&LittleFS, "/menu.dat", 3000);
// PairsFile pairsFile;
// pairsFile.set()